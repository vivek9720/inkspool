#include "inkspool/string_pool.h"

#include <algorithm>
#include <sstream>

namespace inkspool {

Status StringPool::Load(const std::vector<StringEntry>& entries) {
  values_.clear();
  order_.clear();
  for (const auto& entry : entries) {
    if (entry.key.empty()) {
      return Status::Error(ErrorCode::kSemanticError, "empty string key",
                           entry.location.offset, entry.location.line,
                           entry.location.column);
    }
    if (values_.find(entry.key) != values_.end()) {
      return Status::Error(ErrorCode::kDuplicateId,
                           "duplicate string key: " + entry.key,
                           entry.location.offset, entry.location.line,
                           entry.location.column);
    }
    values_[entry.key] = entry.value;
    order_.push_back(entry.key);
  }
  return Status::Ok();
}

const std::string* StringPool::Find(const std::string& key) const {
  auto it = values_.find(key);
  if (it == values_.end()) {
    return nullptr;
  }
  return &it->second;
}

bool StringPool::Contains(const std::string& key) const {
  return values_.find(key) != values_.end();
}

std::vector<std::string> StringPool::Keys() const { return order_; }

std::string StringPool::Describe() const {
  std::ostringstream out;
  out << "strings=" << values_.size();
  if (!order_.empty()) {
    out << " first=" << order_.front();
  }
  return out.str();
}

}  // namespace inkspool

