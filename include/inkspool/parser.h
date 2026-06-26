#ifndef INKSPOOL_PARSER_H
#define INKSPOOL_PARSER_H

#include <cstddef>
#include <string>
#include <vector>

#include "inkspool/byte_view.h"
#include "inkspool/diagnostic.h"
#include "inkspool/document.h"
#include "inkspool/section_table.h"
#include "inkspool/status.h"
#include "inkspool/token.h"

namespace inkspool {

struct ParseOptions {
  std::size_t max_tokens = 100000;
  std::size_t max_operations = 8192;
  bool allow_unknown_operations = true;
  bool collect_sections = true;
};

struct ParseOutput {
  Document document;
  SectionTable sections;
  DiagnosticSink diagnostics;
};

class Parser {
 public:
  Parser(std::vector<Token> tokens, ParseOptions options);
  Result<ParseOutput> Parse();

 private:
  const Token& Peek(std::size_t ahead = 0) const;
  bool AtEnd() const;
  bool Check(TokenKind kind) const;
  bool Match(TokenKind kind);
  bool CheckWord(const std::string& word) const;
  bool MatchWord(const std::string& word);
  Status Expect(TokenKind kind, const std::string& description);
  Result<Token> ExpectValue(const std::string& description);
  Result<std::string> ExpectIdentifier(const std::string& description);
  Result<int> ExpectInt(const std::string& description);
  Status ExpectWord(const std::string& word);
  void SkipNewlines();
  void SkipLine();
  void SynchronizeSection();

  Status ParseMeta();
  Status ParseStrings();
  Status ParseStyles();
  Status ParsePages();
  Status ParseOps();
  Status ParseXref();
  Status ParseOperation(const std::string& page_id, ScriptBlock* script);
  Status ParseOperationFields(Operation* op);
  Status ParseColorToken(Color* color);
  Status RecordSectionBegin(const std::string& name, SourceLocation location);
  Status RecordSectionEnd(const std::string& name, SourceLocation location,
                          std::size_t item_count);

  std::vector<Token> tokens_;
  ParseOptions options_;
  std::size_t pos_ = 0;
  ParseOutput output_;
  std::size_t parsed_operations_ = 0;
};

Result<ParseOutput> Parse(ByteView input, ParseOptions options = {});

}  // namespace inkspool

#endif  // INKSPOOL_PARSER_H

