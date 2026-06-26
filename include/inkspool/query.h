#ifndef INKSPOOL_QUERY_H
#define INKSPOOL_QUERY_H

#include <map>
#include <string>
#include <vector>

#include "inkspool/document.h"
#include "inkspool/indexer.h"

namespace inkspool {

struct QuerySummary {
  std::map<std::string, std::size_t> operation_histogram;
  std::map<std::string, std::size_t> layer_mentions;
  std::vector<XrefResolution> aliases;
  std::vector<std::string> missing_aliases;
  std::size_t text_bytes = 0;
  std::size_t anchor_count = 0;
  std::string Digest() const;
};

class ReferenceScanner {
 public:
  QuerySummary Scan(const Document& document, const DocumentIndex& index) const;

 private:
  void CountOperation(const Operation& op, QuerySummary* summary) const;
};

}  // namespace inkspool

#endif  // INKSPOOL_QUERY_H

