#include "inkspool/page_script.h"

#include <sstream>

namespace inkspool {

void PageScriptMap::Clear() {
  scripts_.clear();
  order_.clear();
}

void PageScriptMap::Add(const ScriptBlock& script) {
  if (scripts_.find(script.page_id) == scripts_.end()) {
    order_.push_back(script.page_id);
  }
  scripts_[script.page_id] = script;
}

const ScriptBlock* PageScriptMap::Find(const std::string& page_id) const {
  auto it = scripts_.find(page_id);
  if (it == scripts_.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<std::string> PageScriptMap::PageIds() const { return order_; }

std::size_t PageScriptMap::OperationCount() const {
  std::size_t total = 0;
  for (const auto& pair : scripts_) {
    total += pair.second.operations.size();
  }
  return total;
}

std::string DescribeOperation(const Operation& op) {
  std::ostringstream out;
  out << OpCodeName(op.code);
  if (!op.layer.empty()) {
    out << " layer=" << op.layer;
  }
  if (!op.id.empty()) {
    out << " id=" << op.id;
  }
  if (!op.style.empty()) {
    out << " style=" << op.style;
  }
  if (!op.string_key.empty()) {
    out << " string=" << op.string_key;
  }
  if (op.x || op.y || op.w || op.h) {
    out << " rect=(" << op.x << "," << op.y << "," << op.w << "," << op.h
        << ")";
  }
  return out.str();
}

}  // namespace inkspool

