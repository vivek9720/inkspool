#ifndef INKSPOOL_SECTION_TABLE_H
#define INKSPOOL_SECTION_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/diagnostic.h"

namespace inkspool {

struct SectionRecord {
  std::string name;
  SourceLocation start;
  SourceLocation end;
  std::size_t item_count = 0;
};

class SectionTable {
 public:
  void Clear();
  void Begin(const std::string& name, SourceLocation location);
  void End(const std::string& name, SourceLocation location,
           std::size_t item_count);
  bool Seen(const std::string& name) const;
  const SectionRecord* Find(const std::string& name) const;
  const std::vector<SectionRecord>& records() const { return records_; }
  std::string Summary() const;

 private:
  std::vector<SectionRecord> records_;
  std::unordered_map<std::string, std::size_t> latest_;
};

}  // namespace inkspool

#endif  // INKSPOOL_SECTION_TABLE_H

