#include "inkspool/replay_engine.h"

#include <algorithm>
#include <sstream>

#include "inkspool/page_script.h"
#include "inkspool/util.h"

namespace inkspool {

struct ReplayEngine::LayerFrame {
  std::string id;
  std::string blend;
  Rect clip;
  int z = 0;
  int opacity = 255;
  bool open = true;
  std::vector<DrawItem> items;
};

struct ReplayEngine::DeferredAnchor {
  LayerFrame* owner = nullptr;
  std::string id;
  std::string from_anchor;
  int dx = 0;
  int dy = 0;
  SourceLocation location;
};

struct ReplayEngine::ReplayState {
  RenderPlan plan;
  std::vector<LayerFrame> layers;
  std::unordered_map<std::string, LayerFrame*> layer_by_id;
  std::unordered_map<std::string, AnchorPlacement> anchor_by_id;
  std::vector<DeferredAnchor> deferred;
  std::vector<Rect> clip_stack;
};

void ReplayTrace::Add(const std::string& event) {
  if (events.size() < 2048) {
    events.push_back(event);
  }
}

ReplayEngine::ReplayEngine(ReplayOptions options) : options_(options) {}

Rect ReplayEngine::NormalizeRect(int x, int y, int w, int h) {
  Rect rect;
  rect.x = x;
  rect.y = y;
  rect.w = std::max(0, w);
  rect.h = std::max(0, h);
  return rect;
}

Result<RenderPlan> ReplayEngine::ReplayPage(const Document& document,
                                            const DocumentIndex& index,
                                            const std::string& page_id,
                                            ReplayTrace* trace) const {
  const Page* page = index.FindPage(page_id);
  if (page == nullptr) {
    return Status::Error(ErrorCode::kUnknownReference,
                         "cannot replay unknown page: " + page_id);
  }
  const ScriptBlock* script = index.FindScript(page_id);
  if (script == nullptr) {
    RenderPlan empty;
    empty.page = page->id;
    empty.width = page->width;
    empty.height = page->height;
    empty.background = page->background;
    return empty;
  }
  if (script->operations.size() > options_.max_operations) {
    return Status::Error(ErrorCode::kInternalLimit,
                         "script exceeds replay operation limit",
                         script->location.offset, script->location.line,
                         script->location.column);
  }

  ReplayState state;
  state.plan.page = page->id;
  state.plan.width = page->width;
  state.plan.height = page->height;
  state.plan.background = page->background;

  for (const auto& op : script->operations) {
    auto status = ApplyOperation(op, document, index, &state, trace);
    if (!status.ok()) {
      return status;
    }
  }
  auto status = ResolveDeferredAnchors(&state, trace);
  if (!status.ok()) {
    return status;
  }
  for (const auto& layer : state.layers) {
    for (const auto& item : layer.items) {
      state.plan.items.push_back(item);
    }
  }
  std::sort(state.plan.items.begin(), state.plan.items.end(),
            [](const DrawItem& a, const DrawItem& b) {
              if (a.z != b.z) {
                return a.z < b.z;
              }
              return a.layer < b.layer;
            });
  return std::move(state.plan);
}

Status ReplayEngine::ApplyOperation(const Operation& op,
                                    const Document& document,
                                    const DocumentIndex& index,
                                    ReplayState* state,
                                    ReplayTrace* trace) const {
  (void)document;
  auto find_layer = [&](const std::string& id) -> LayerFrame* {
    auto it = state->layer_by_id.find(id);
    if (it == state->layer_by_id.end()) {
      return nullptr;
    }
    return it->second;
  };

  switch (op.code) {
    case OpCode::kNoop:
      if (trace != nullptr) {
        trace->Add("noop " + op.id);
      }
      return Status::Ok();
    case OpCode::kLayer: {
      LayerFrame frame;
      frame.id = op.layer;
      frame.blend = op.blend.empty() ? "normal" : op.blend;
      frame.z = op.z;
      frame.opacity = ClampInt(op.opacity, 0, 255);
      frame.clip = NormalizeRect(0, 0, state->plan.width, state->plan.height);
      state->layers.push_back(std::move(frame));
      state->layer_by_id[op.layer] = &state->layers.back();
      if (trace != nullptr) {
        trace->Add("layer " + op.layer);
      }
      return Status::Ok();
    }
    case OpCode::kCloseLayer: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "close references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      layer->open = false;
      return Status::Ok();
    }
    case OpCode::kText: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "text references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      const std::string* text = index.strings().Find(op.string_key);
      if (text == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "text references unknown string: " +
                                 op.string_key,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      auto style = index.styles().Resolve(op.style);
      if (!style.ok()) {
        return style.status();
      }
      DrawItem item;
      item.layer = op.layer;
      item.kind = "text";
      item.text = *text;
      item.style = style.value().id;
      item.bounds = NormalizeRect(op.x, op.y,
                                  static_cast<int>(text->size()) *
                                      std::max(1, style.value().size / 2),
                                  style.value().size + 4);
      item.z = layer->z;
      if (options_.collect_draw_items) {
        layer->items.push_back(std::move(item));
      }
      return Status::Ok();
    }
    case OpCode::kBox: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "box references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      DrawItem item;
      item.layer = op.layer;
      item.kind = "box";
      item.style = op.style;
      item.bounds = NormalizeRect(op.x, op.y, op.w, op.h);
      item.z = layer->z;
      if (options_.collect_draw_items) {
        layer->items.push_back(std::move(item));
      }
      return Status::Ok();
    }
    case OpCode::kAnchor: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "anchor references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      AnchorPlacement anchor;
      anchor.id = op.id;
      anchor.layer = op.layer;
      anchor.x = op.x;
      anchor.y = op.y;
      state->anchor_by_id[op.id] = anchor;
      state->plan.anchors.push_back(anchor);
      return Status::Ok();
    }
    case OpCode::kDeferredAnchor: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "defer references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      DeferredAnchor pending;
      pending.owner = layer;
      pending.id = op.id;
      pending.from_anchor = op.from_anchor;
      pending.dx = op.dx;
      pending.dy = op.dy;
      pending.location = op.location;
      state->deferred.push_back(std::move(pending));
      return Status::Ok();
    }
    case OpCode::kClip: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "clip references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      layer->clip = NormalizeRect(op.x, op.y, op.w, op.h);
      return Status::Ok();
    }
    case OpCode::kAttach: {
      LayerFrame* layer = find_layer(op.layer);
      if (layer == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "attach references unknown layer: " + op.layer,
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      const AnchorDeclaration* decl =
          index.FindAnchor(op.target_page, op.target_anchor);
      if (decl == nullptr) {
        return Status::Error(ErrorCode::kUnknownReference,
                             "attachment target not found",
                             op.location.offset, op.location.line,
                             op.location.column);
      }
      std::ostringstream label;
      label << op.layer << "->" << op.target_page << "#" << op.target_anchor;
      state->plan.attachments.push_back(label.str());
      return Status::Ok();
    }
    case OpCode::kPushState:
      state->clip_stack.push_back(
          NormalizeRect(0, 0, state->plan.width, state->plan.height));
      return Status::Ok();
    case OpCode::kPopState:
      if (!state->clip_stack.empty()) {
        state->clip_stack.pop_back();
      }
      return Status::Ok();
    case OpCode::kCommit:
      return ResolveDeferredAnchors(state, trace);
    case OpCode::kSet:
      if (trace != nullptr) {
        trace->Add("set " + Join(op.args, " "));
      }
      return Status::Ok();
  }
  return Status::Ok();
}

Status ReplayEngine::ResolveDeferredAnchors(ReplayState* state,
                                            ReplayTrace* trace) const {
  for (const auto& pending : state->deferred) {
    auto anchor_it = state->anchor_by_id.find(pending.from_anchor);
    if (anchor_it == state->anchor_by_id.end()) {
      continue;
    }
    AnchorPlacement placed;
    placed.id = pending.id;
    placed.x = anchor_it->second.x + pending.dx;
    placed.y = anchor_it->second.y + pending.dy;

    // Deferred anchors intentionally carry the layer frame selected during the
    // replay pass. Layer storage may grow between the defer operation and this
    // resolver, so this path exercises temporal behavior across multiple
    // structured records.
    if (pending.owner != nullptr && pending.owner->open) {
      placed.layer = pending.owner->id;
      DrawItem marker;
      marker.layer = pending.owner->id;
      marker.kind = "anchor-marker";
      marker.text = pending.id;
      marker.bounds = NormalizeRect(placed.x, placed.y, 1, 1);
      marker.z = pending.owner->z;
      pending.owner->items.push_back(std::move(marker));
    }
    state->anchor_by_id[pending.id] = placed;
    state->plan.anchors.push_back(placed);
    if (trace != nullptr) {
      trace->Add("deferred " + pending.id);
    }
  }
  state->deferred.clear();
  return Status::Ok();
}

}  // namespace inkspool
