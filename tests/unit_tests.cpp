#include <cstdlib>
#include <iostream>
#include <string>

#include "inkspool/format_writer.h"
#include "inkspool/indexer.h"
#include "inkspool/parser.h"
#include "inkspool/query.h"
#include "inkspool/replay_engine.h"
#include "inkspool/validator.h"

namespace {

int failures = 0;

void Expect(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << "\n";
  }
}

inkspool::Result<inkspool::ParseOutput> ParseText(const std::string& text) {
  return inkspool::Parse(inkspool::ByteView(
      reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
}

const char* kValid = R"(INKSPOOL/1
meta title "unit"
strings {
  entry title = "Hello"
  entry body = "Body copy"
}
styles {
  style base font main size 12 color #101820 flags regular
  style heading font main size 18 color #da4167 inherit base flags bold
}
pages {
  page cover width 640 height 480 background #f7f7f7 label "Cover"
}
ops cover {
  layer root z 0 blend normal
  text root style heading at 16 24 string title
  anchor root top at 16 24
  defer root lower from top dx 0 dy 24
  commit
}
xref {
  alias start cover top
}
END
)";

void TestParseAndReplay() {
  auto parsed = ParseText(kValid);
  Expect(parsed.ok(), "valid document parses");
  if (!parsed.ok()) {
    std::cerr << parsed.status().ToString() << "\n";
    return;
  }
  inkspool::Validator validator;
  auto report = validator.Validate(parsed.value().document);
  Expect(report.ok(), "valid document validates");
  inkspool::DocumentIndex index;
  auto status = index.Build(parsed.value().document);
  Expect(status.ok(), "index builds");
  inkspool::ReplayEngine engine;
  auto plan = engine.ReplayPage(parsed.value().document, index, "cover");
  Expect(plan.ok(), "render plan builds");
  if (plan.ok()) {
    Expect(plan.value().items.size() >= 2, "plan has text and deferred marker");
    Expect(plan.value().anchors.size() == 2, "plan has direct and deferred anchors");
  }
}

void TestSemanticError() {
  const std::string text = R"(INKSPOOL/1
strings {
  entry title = "Hello"
}
styles {
  style base font main size 12 color #101820
}
pages {
  page cover width 100 height 100 background #ffffff
}
ops cover {
  layer root z 0 blend normal
  text root style missing at 1 2 string title
}
xref {
}
END
)";
  auto parsed = ParseText(text);
  Expect(parsed.ok(), "bad semantic document still parses");
  if (!parsed.ok()) {
    return;
  }
  inkspool::Validator validator;
  auto report = validator.Validate(parsed.value().document);
  Expect(!report.ok(), "validator catches missing style");
}

void TestWriterAndQuery() {
  auto parsed = ParseText(kValid);
  Expect(parsed.ok(), "writer fixture parses");
  if (!parsed.ok()) {
    return;
  }
  inkspool::DocumentIndex index;
  auto status = index.Build(parsed.value().document);
  Expect(status.ok(), "query index builds");
  inkspool::ReferenceScanner scanner;
  auto summary = scanner.Scan(parsed.value().document, index);
  Expect(summary.operation_histogram["text"] == 1, "query counts text");
  Expect(summary.aliases.size() == 1, "query records alias");
  inkspool::FormatWriter writer;
  const std::string normalized = writer.Write(parsed.value().document);
  Expect(normalized.find("INKSPOOL/1") == 0, "writer includes magic");
  auto reparsed = ParseText(normalized);
  Expect(reparsed.ok(), "normalized document reparses");
}

void TestParserRejectsBadMagic() {
  auto parsed = ParseText("NOPE\n");
  Expect(!parsed.ok(), "bad magic rejected");
}

}  // namespace

int main() {
  TestParseAndReplay();
  TestSemanticError();
  TestWriterAndQuery();
  TestParserRejectsBadMagic();
  if (failures != 0) {
    std::cerr << failures << " test failure(s)\n";
    return 1;
  }
  std::cout << "all tests passed\n";
  return 0;
}

