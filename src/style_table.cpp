#include "inkspool/style_table.h"

#include <algorithm>

namespace inkspool {

Status StyleTable::Load(const std::vector<Style>& styles) {
  styles_.clear();
  order_.clear();
  for (const auto& style : styles) {
    if (style.id.empty()) {
      return Status::Error(ErrorCode::kSemanticError, "empty style id",
                           style.location.offset, style.location.line,
                           style.location.column);
    }
    if (styles_.find(style.id) != styles_.end()) {
      return Status::Error(ErrorCode::kDuplicateId,
                           "duplicate style id: " + style.id,
                           style.location.offset, style.location.line,
                           style.location.column);
    }
    styles_[style.id] = style;
    order_.push_back(style.id);
  }
  return Status::Ok();
}

const Style* StyleTable::FindRaw(const std::string& id) const {
  auto it = styles_.find(id);
  if (it == styles_.end()) {
    return nullptr;
  }
  return &it->second;
}

Result<ResolvedStyle> StyleTable::Resolve(const std::string& id) const {
  std::vector<std::string> stack;
  return ResolveInternal(id, &stack);
}

Result<ResolvedStyle> StyleTable::ResolveInternal(
    const std::string& id, std::vector<std::string>* stack) const {
  auto it = styles_.find(id);
  if (it == styles_.end()) {
    return Status::Error(ErrorCode::kUnknownReference,
                         "unknown style: " + id);
  }
  if (std::find(stack->begin(), stack->end(), id) != stack->end()) {
    return Status::Error(ErrorCode::kSemanticError,
                         "style inheritance cycle at: " + id);
  }
  stack->push_back(id);
  const Style& style = it->second;
  ResolvedStyle resolved;
  if (!style.inherit.empty()) {
    auto parent = ResolveInternal(style.inherit, stack);
    if (!parent.ok()) {
      return parent.status();
    }
    resolved = parent.TakeValue();
  }
  resolved.id = style.id;
  if (!style.font.empty()) {
    resolved.font = style.font;
  }
  resolved.color = style.color;
  resolved.size = style.size;
  resolved.tracking = style.tracking;
  for (const auto& flag : style.flags) {
    if (std::find(resolved.flags.begin(), resolved.flags.end(), flag) ==
        resolved.flags.end()) {
      resolved.flags.push_back(flag);
    }
  }
  stack->pop_back();
  return resolved;
}

bool StyleTable::Contains(const std::string& id) const {
  return styles_.find(id) != styles_.end();
}

std::vector<std::string> StyleTable::Ids() const { return order_; }

}  // namespace inkspool

