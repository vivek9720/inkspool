#ifndef INKSPOOL_PAGE_SCRIPT_H
#define INKSPOOL_PAGE_SCRIPT_H

#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/document.h"

namespace inkspool {

class PageScriptMap {
 public:
  void Clear();
  void Add(const ScriptBlock& script);
  const ScriptBlock* Find(const std::string& page_id) const;
  std::vector<std::string> PageIds() const;
  std::size_t OperationCount() const;

 private:
  std::unordered_map<std::string, ScriptBlock> scripts_;
  std::vector<std::string> order_;
};

std::string DescribeOperation(const Operation& op);

}  // namespace inkspool

#endif  // INKSPOOL_PAGE_SCRIPT_H

