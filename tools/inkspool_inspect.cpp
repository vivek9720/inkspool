#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "inkspool/checksum.h"
#include "inkspool/format_writer.h"
#include "inkspool/indexer.h"
#include "inkspool/parser.h"
#include "inkspool/query.h"
#include "inkspool/replay_engine.h"
#include "inkspool/validator.h"

namespace {

std::vector<std::uint8_t> ReadFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>());
}

void Usage() {
  std::cerr << "usage: inkspool_inspect <file> [--summary|--validate|--dump|"
               "--plan <page>|--query]\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    Usage();
    return 2;
  }
  const std::string path = argv[1];
  std::string mode = "--summary";
  std::string page_id;
  if (argc >= 3) {
    mode = argv[2];
  }
  if (mode == "--plan") {
    if (argc < 4) {
      Usage();
      return 2;
    }
    page_id = argv[3];
  }

  auto bytes = ReadFile(path);
  if (bytes.empty()) {
    std::cerr << "failed to read input or file is empty\n";
    return 1;
  }
  auto parsed = inkspool::Parse(
      inkspool::ByteView(bytes.data(), bytes.size()), inkspool::ParseOptions{});
  if (!parsed.ok()) {
    std::cerr << parsed.status().ToString() << "\n";
    return 1;
  }
  const auto& document = parsed.value().document;
  inkspool::Validator validator;
  auto report = validator.Validate(document);
  if (!report.ok()) {
    std::cerr << report.diagnostics.FormatAll();
    return 1;
  }
  if (mode == "--validate") {
    std::cout << "ok\n";
    return 0;
  }

  inkspool::DocumentIndex index;
  auto status = index.Build(document);
  if (!status.ok()) {
    std::cerr << status.ToString() << "\n";
    return 1;
  }

  if (mode == "--dump") {
    inkspool::FormatWriter writer;
    std::cout << writer.Write(document);
    return 0;
  }
  if (mode == "--query") {
    inkspool::ReferenceScanner scanner;
    auto summary = scanner.Scan(document, index);
    std::cout << summary.Digest() << "\n";
    return 0;
  }
  if (mode == "--plan") {
    inkspool::ReplayEngine engine;
    auto plan = engine.ReplayPage(document, index, page_id);
    if (!plan.ok()) {
      std::cerr << plan.status().ToString() << "\n";
      return 1;
    }
    std::cout << plan.value().Summary() << "\n";
    for (const auto& item : plan.value().items) {
      std::cout << "item " << item.kind << " layer=" << item.layer
                << " bounds=" << item.bounds.x << "," << item.bounds.y << ","
                << item.bounds.w << "," << item.bounds.h << "\n";
    }
    return 0;
  }

  inkspool::FormatWriter writer;
  const std::string normalized = writer.Write(document);
  std::cout << index.Summary() << "\n";
  std::cout << "operations=" << document.OperationCount() << "\n";
  std::cout << "digest=" << inkspool::StableDocumentDigest(normalized) << "\n";
  std::cout << "sections:\n" << parsed.value().sections.Summary();
  return 0;
}

