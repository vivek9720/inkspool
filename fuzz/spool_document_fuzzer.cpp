#include <cstddef>
#include <cstdint>
#include <string>

#include "inkspool/checksum.h"
#include "inkspool/format_writer.h"
#include "inkspool/indexer.h"
#include "inkspool/parser.h"
#include "inkspool/query.h"
#include "inkspool/replay_engine.h"
#include "inkspool/validator.h"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
  inkspool::ParseOptions parse_options;
  parse_options.max_tokens = 60000;
  parse_options.max_operations = 4096;
  auto parsed = inkspool::Parse(inkspool::ByteView(data, size), parse_options);
  if (!parsed.ok()) {
    return 0;
  }

  inkspool::Validator validator({20000u * 20000u, 4096, false});
  auto report = validator.Validate(parsed.value().document);
  if (!report.ok()) {
    return 0;
  }

  inkspool::DocumentIndex index;
  auto status = index.Build(parsed.value().document);
  if (!status.ok()) {
    return 0;
  }

  inkspool::ReplayEngine engine({4096, true, false});
  for (const auto& page_id : index.PageIds()) {
    inkspool::ReplayTrace trace;
    auto plan = engine.ReplayPage(parsed.value().document, index, page_id, &trace);
    if (plan.ok()) {
      std::string summary = plan.value().Summary();
      (void)inkspool::Fnv1a64(summary);
    }
  }

  inkspool::ReferenceScanner scanner;
  auto summary = scanner.Scan(parsed.value().document, index);
  (void)summary.Digest();
  inkspool::FormatWriter writer;
  (void)inkspool::StableDocumentDigest(writer.Write(parsed.value().document));
  return 0;
}

