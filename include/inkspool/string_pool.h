#ifndef INKSPOOL_STRING_POOL_H
#define INKSPOOL_STRING_POOL_H

#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/document.h"
#include "inkspool/status.h"

namespace inkspool {

class StringPool {
 public:
  Status Load(const std::vector<StringEntry>& entries);
  const std::string* Find(const std::string& key) const;
  bool Contains(const std::string& key) const;
  std::vector<std::string> Keys() const;
  std::string Describe() const;

 private:
  std::unordered_map<std::string, std::string> values_;
  std::vector<std::string> order_;
};

}  // namespace inkspool

#endif  // INKSPOOL_STRING_POOL_H

