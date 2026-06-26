#include "inkspool/query.h"

#include <sstream>

#include "inkspool/page_script.h"

namespace inkspool {

std::string QuerySummary::Digest() const {
  std::ostringstream out;
  out << "ops=";
  for (const auto& pair : operation_histogram) {
    out << pair.first << ":" << pair.second << ",";
  }
  out << " layers=" << layer_mentions.size() << " aliases=" << aliases.size()
      << " missing=" << missing_aliases.size() << " text=" << text_bytes
      << " anchors=" << anchor_count;
  return out.str();
}

QuerySummary ReferenceScanner::Scan(const Document& document,
                                    const DocumentIndex& index) const {
  QuerySummary summary;
  for (const auto& entry : document.strings) {
    summary.text_bytes += entry.value.size();
  }
  for (const auto& script : document.scripts) {
    for (const auto& op : script.operations) {
      CountOperation(op, &summary);
      if (op.code == OpCode::kAnchor) {
        ++summary.anchor_count;
      }
    }
    for (const auto& anchor : index.AnchorIds(script.page_id)) {
      (void)anchor;
    }
  }
  for (const auto& xref : document.xrefs) {
    auto resolution = index.ResolveAlias(xref.alias);
    if (!resolution.found) {
      summary.missing_aliases.push_back(xref.alias);
    }
    summary.aliases.push_back(std::move(resolution));
  }
  return summary;
}

void ReferenceScanner::CountOperation(const Operation& op,
                                      QuerySummary* summary) const {
  summary->operation_histogram[OpCodeName(op.code)] += 1;
  if (!op.layer.empty()) {
    summary->layer_mentions[op.layer] += 1;
  }
  if (!op.target_page.empty()) {
    summary->layer_mentions["page:" + op.target_page] += 1;
  }
}

}  // namespace inkspool

