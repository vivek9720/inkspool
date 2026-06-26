#include "inkspool/indexer.h"

#include <algorithm>
#include <sstream>

namespace inkspool {

std::string AnchorKey(const std::string& page, const std::string& anchor) {
  return page + "\x1f" + anchor;
}

void DocumentIndex::Clear() {
  strings_ = StringPool();
  styles_ = StyleTable();
  scripts_.Clear();
  pages_.clear();
  page_order_.clear();
  anchors_.clear();
  aliases_.clear();
}

Status DocumentIndex::Build(const Document& document) {
  Clear();
  auto status = strings_.Load(document.strings);
  if (!status.ok()) {
    return status;
  }
  status = styles_.Load(document.styles);
  if (!status.ok()) {
    return status;
  }
  for (const auto& page : document.pages) {
    if (pages_.find(page.id) != pages_.end()) {
      return Status::Error(ErrorCode::kDuplicateId,
                           "duplicate page id while indexing: " + page.id,
                           page.location.offset, page.location.line,
                           page.location.column);
    }
    pages_[page.id] = page;
    page_order_.push_back(page.id);
  }
  for (const auto& script : document.scripts) {
    scripts_.Add(script);
    for (const auto& op : script.operations) {
      if (op.code != OpCode::kAnchor) {
        continue;
      }
      AnchorDeclaration decl;
      decl.page = script.page_id;
      decl.layer = op.layer;
      decl.id = op.id;
      decl.x = op.x;
      decl.y = op.y;
      decl.location = op.location;
      const std::string key = AnchorKey(script.page_id, op.id);
      if (anchors_.find(key) == anchors_.end()) {
        anchors_[key] = std::move(decl);
      }
    }
  }
  for (const auto& xref : document.xrefs) {
    aliases_[xref.alias] = xref;
  }
  return Status::Ok();
}

const Page* DocumentIndex::FindPage(const std::string& id) const {
  auto it = pages_.find(id);
  if (it == pages_.end()) {
    return nullptr;
  }
  return &it->second;
}

const ScriptBlock* DocumentIndex::FindScript(const std::string& page_id) const {
  return scripts_.Find(page_id);
}

const AnchorDeclaration* DocumentIndex::FindAnchor(
    const std::string& page, const std::string& id) const {
  auto it = anchors_.find(AnchorKey(page, id));
  if (it == anchors_.end()) {
    return nullptr;
  }
  return &it->second;
}

XrefResolution DocumentIndex::ResolveAlias(const std::string& alias) const {
  XrefResolution resolution;
  resolution.alias = alias;
  auto it = aliases_.find(alias);
  if (it == aliases_.end()) {
    return resolution;
  }
  resolution.page = it->second.page;
  resolution.anchor = it->second.anchor;
  resolution.found = FindAnchor(resolution.page, resolution.anchor) != nullptr;
  return resolution;
}

std::vector<std::string> DocumentIndex::PageIds() const { return page_order_; }

std::vector<std::string> DocumentIndex::AnchorIds(const std::string& page) const {
  std::vector<std::string> ids;
  const std::string prefix = page + "\x1f";
  for (const auto& pair : anchors_) {
    if (pair.first.compare(0, prefix.size(), prefix) == 0) {
      ids.push_back(pair.second.id);
    }
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

std::string DocumentIndex::Summary() const {
  std::ostringstream out;
  out << "pages=" << pages_.size() << " scripts=" << scripts_.PageIds().size()
      << " anchors=" << anchors_.size() << " aliases=" << aliases_.size()
      << " " << strings_.Describe();
  return out.str();
}

}  // namespace inkspool

