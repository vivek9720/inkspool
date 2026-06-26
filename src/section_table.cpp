#include "inkspool/section_table.h"

#include <sstream>

namespace inkspool {

void SectionTable::Clear() {
  records_.clear();
  latest_.clear();
}

void SectionTable::Begin(const std::string& name, SourceLocation location) {
  SectionRecord record;
  record.name = name;
  record.start = location;
  records_.push_back(record);
  latest_[name] = records_.size() - 1;
}

void SectionTable::End(const std::string& name, SourceLocation location,
                       std::size_t item_count) {
  auto it = latest_.find(name);
  if (it == latest_.end()) {
    Begin(name, location);
    it = latest_.find(name);
  }
  records_[it->second].end = location;
  records_[it->second].item_count = item_count;
}

bool SectionTable::Seen(const std::string& name) const {
  return latest_.find(name) != latest_.end();
}

const SectionRecord* SectionTable::Find(const std::string& name) const {
  auto it = latest_.find(name);
  if (it == latest_.end()) {
    return nullptr;
  }
  return &records_[it->second];
}

std::string SectionTable::Summary() const {
  std::ostringstream out;
  for (const auto& record : records_) {
    out << record.name << " items=" << record.item_count << " lines="
        << record.start.line << "-" << record.end.line << "\n";
  }
  return out.str();
}

}  // namespace inkspool

