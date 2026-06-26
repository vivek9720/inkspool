#include "inkspool/parser.h"

#include <algorithm>
#include <sstream>

#include "inkspool/lexer.h"
#include "inkspool/util.h"

namespace inkspool {

namespace {

Status TokenError(const Token& token, ErrorCode code,
                  const std::string& message) {
  return Status::Error(code, message, token.location.offset,
                       token.location.line, token.location.column);
}

std::string TokenValue(const Token& token) {
  if (token.kind == TokenKind::kString || token.kind == TokenKind::kIdentifier ||
      token.kind == TokenKind::kNumber) {
    return token.text;
  }
  return TokenKindName(token.kind);
}

}  // namespace

Parser::Parser(std::vector<Token> tokens, ParseOptions options)
    : tokens_(std::move(tokens)), options_(options) {}

Result<ParseOutput> Parse(ByteView input, ParseOptions options) {
  Lexer lexer(input);
  auto tokens = lexer.Tokenize();
  if (!tokens.ok()) {
    return tokens.status();
  }
  if (tokens.value().size() > options.max_tokens) {
    return Status::Error(ErrorCode::kInternalLimit, "token limit exceeded");
  }
  Parser parser(tokens.TakeValue(), options);
  return parser.Parse();
}

const Token& Parser::Peek(std::size_t ahead) const {
  const std::size_t index = std::min(pos_ + ahead, tokens_.size() - 1);
  return tokens_[index];
}

bool Parser::AtEnd() const { return Peek().kind == TokenKind::kEndOfFile; }

bool Parser::Check(TokenKind kind) const { return Peek().kind == kind; }

bool Parser::Match(TokenKind kind) {
  if (!Check(kind)) {
    return false;
  }
  ++pos_;
  return true;
}

bool Parser::CheckWord(const std::string& word) const {
  const Token& token = Peek();
  return (token.kind == TokenKind::kIdentifier || token.kind == TokenKind::kString) &&
         token.text == word;
}

bool Parser::MatchWord(const std::string& word) {
  if (!CheckWord(word)) {
    return false;
  }
  ++pos_;
  return true;
}

Status Parser::Expect(TokenKind kind, const std::string& description) {
  if (!Match(kind)) {
    std::ostringstream message;
    message << "expected " << description << ", got " << TokenKindName(Peek().kind);
    return TokenError(Peek(), ErrorCode::kSyntaxError, message.str());
  }
  return Status::Ok();
}

Result<Token> Parser::ExpectValue(const std::string& description) {
  const Token& token = Peek();
  if (token.kind == TokenKind::kIdentifier || token.kind == TokenKind::kString ||
      token.kind == TokenKind::kNumber) {
    ++pos_;
    return token;
  }
  return TokenError(token, ErrorCode::kSyntaxError,
                    "expected " + description + ", got " +
                        TokenKindName(token.kind));
}

Result<std::string> Parser::ExpectIdentifier(const std::string& description) {
  auto token = ExpectValue(description);
  if (!token.ok()) {
    return token.status();
  }
  return TokenValue(token.value());
}

Result<int> Parser::ExpectInt(const std::string& description) {
  auto token = ExpectValue(description);
  if (!token.ok()) {
    return token.status();
  }
  if (token.value().kind == TokenKind::kNumber) {
    if (token.value().number < -2147483647LL - 1LL ||
        token.value().number > 2147483647LL) {
      return TokenError(token.value(), ErrorCode::kRangeError,
                        "integer out of range for " + description);
    }
    return static_cast<int>(token.value().number);
  }
  auto parsed = ParseIntStrict(token.value().text);
  if (!parsed.ok()) {
    return TokenError(token.value(), parsed.status().code(),
                      parsed.status().message());
  }
  return parsed.TakeValue();
}

Status Parser::ExpectWord(const std::string& word) {
  if (!MatchWord(word)) {
    return TokenError(Peek(), ErrorCode::kSyntaxError,
                      "expected keyword '" + word + "'");
  }
  return Status::Ok();
}

void Parser::SkipNewlines() {
  while (Match(TokenKind::kNewline)) {
  }
}

void Parser::SkipLine() {
  while (!AtEnd() && !Check(TokenKind::kNewline) && !Check(TokenKind::kRBrace)) {
    ++pos_;
  }
  Match(TokenKind::kNewline);
}

void Parser::SynchronizeSection() {
  while (!AtEnd()) {
    if (Match(TokenKind::kRBrace)) {
      return;
    }
    ++pos_;
  }
}

Status Parser::RecordSectionBegin(const std::string& name,
                                  SourceLocation location) {
  if (options_.collect_sections) {
    output_.sections.Begin(name, location);
  }
  return Status::Ok();
}

Status Parser::RecordSectionEnd(const std::string& name, SourceLocation location,
                                std::size_t item_count) {
  if (options_.collect_sections) {
    output_.sections.End(name, location, item_count);
  }
  return Status::Ok();
}

Result<ParseOutput> Parser::Parse() {
  SkipNewlines();
  if (!MatchWord("INKSPOOL/1")) {
    return TokenError(Peek(), ErrorCode::kBadMagic,
                      "missing INKSPOOL/1 envelope");
  }
  SkipLine();

  while (!AtEnd()) {
    SkipNewlines();
    if (AtEnd()) {
      break;
    }
    if (MatchWord("END")) {
      SkipLine();
      break;
    }
    Status status = Status::Ok();
    if (CheckWord("meta")) {
      status = ParseMeta();
    } else if (CheckWord("strings")) {
      status = ParseStrings();
    } else if (CheckWord("styles")) {
      status = ParseStyles();
    } else if (CheckWord("pages")) {
      status = ParsePages();
    } else if (CheckWord("ops")) {
      status = ParseOps();
    } else if (CheckWord("xref")) {
      status = ParseXref();
    } else {
      status = TokenError(Peek(), ErrorCode::kSyntaxError,
                          "unknown top-level section: " + TokenValue(Peek()));
    }
    if (!status.ok()) {
      output_.diagnostics.AddError(status.code(), status.message(),
                                   Peek().location);
      return status;
    }
  }
  return std::move(output_);
}

Status Parser::ParseMeta() {
  const SourceLocation location = Peek().location;
  MatchWord("meta");
  auto key = ExpectIdentifier("metadata key");
  if (!key.ok()) {
    return key.status();
  }
  auto value = ExpectValue("metadata value");
  if (!value.ok()) {
    return value.status();
  }
  output_.document.metadata.fields[key.value()] = TokenValue(value.value());
  output_.document.metadata.location = location;
  SkipLine();
  return Status::Ok();
}

Status Parser::ParseStrings() {
  const SourceLocation begin = Peek().location;
  MatchWord("strings");
  auto status = Expect(TokenKind::kLBrace, "strings block");
  if (!status.ok()) {
    return status;
  }
  RecordSectionBegin("strings", begin);
  std::size_t count = 0;
  while (!AtEnd() && !Check(TokenKind::kRBrace)) {
    SkipNewlines();
    if (Check(TokenKind::kRBrace) || AtEnd()) {
      break;
    }
    MatchWord("entry");
    MatchWord("string");
    const SourceLocation loc = Peek().location;
    auto key = ExpectIdentifier("string key");
    if (!key.ok()) {
      return key.status();
    }
    status = Expect(TokenKind::kEqual, "=");
    if (!status.ok()) {
      return status;
    }
    auto value = ExpectValue("string value");
    if (!value.ok()) {
      return value.status();
    }
    StringEntry entry;
    entry.key = key.TakeValue();
    entry.value = TokenValue(value.value());
    entry.location = loc;
    while (!AtEnd() && !Check(TokenKind::kNewline) && !Check(TokenKind::kRBrace)) {
      if (MatchWord("flags")) {
        continue;
      }
      auto flag = ExpectIdentifier("string flag");
      if (!flag.ok()) {
        return flag.status();
      }
      entry.flags.push_back(flag.TakeValue());
    }
    output_.document.strings.push_back(std::move(entry));
    ++count;
    SkipLine();
  }
  SourceLocation end = Peek().location;
  status = Expect(TokenKind::kRBrace, "}");
  if (!status.ok()) {
    return status;
  }
  SkipLine();
  return RecordSectionEnd("strings", end, count);
}

Status Parser::ParseColorToken(Color* color) {
  auto value = ExpectValue("color literal");
  if (!value.ok()) {
    return value.status();
  }
  auto parsed = ParseColor(TokenValue(value.value()));
  if (!parsed.ok()) {
    return TokenError(value.value(), parsed.status().code(),
                      parsed.status().message());
  }
  *color = parsed.TakeValue();
  return Status::Ok();
}

Status Parser::ParseStyles() {
  const SourceLocation begin = Peek().location;
  MatchWord("styles");
  auto status = Expect(TokenKind::kLBrace, "styles block");
  if (!status.ok()) {
    return status;
  }
  RecordSectionBegin("styles", begin);
  std::size_t count = 0;
  while (!AtEnd() && !Check(TokenKind::kRBrace)) {
    SkipNewlines();
    if (Check(TokenKind::kRBrace) || AtEnd()) {
      break;
    }
    const SourceLocation loc = Peek().location;
    status = ExpectWord("style");
    if (!status.ok()) {
      return status;
    }
    auto id = ExpectIdentifier("style id");
    if (!id.ok()) {
      return id.status();
    }
    Style style;
    style.id = id.TakeValue();
    style.location = loc;
    while (!AtEnd() && !Check(TokenKind::kNewline) && !Check(TokenKind::kRBrace)) {
      if (MatchWord("font")) {
        auto font = ExpectIdentifier("font id");
        if (!font.ok()) {
          return font.status();
        }
        style.font = font.TakeValue();
      } else if (MatchWord("size")) {
        auto size = ExpectInt("style size");
        if (!size.ok()) {
          return size.status();
        }
        style.size = size.TakeValue();
      } else if (MatchWord("tracking")) {
        auto tracking = ExpectInt("tracking");
        if (!tracking.ok()) {
          return tracking.status();
        }
        style.tracking = tracking.TakeValue();
      } else if (MatchWord("color")) {
        status = ParseColorToken(&style.color);
        if (!status.ok()) {
          return status;
        }
      } else if (MatchWord("inherit")) {
        auto inherit = ExpectIdentifier("parent style id");
        if (!inherit.ok()) {
          return inherit.status();
        }
        style.inherit = inherit.TakeValue();
      } else if (MatchWord("flags")) {
        while (!AtEnd() && !Check(TokenKind::kNewline) &&
               !Check(TokenKind::kRBrace)) {
          auto flag = ExpectIdentifier("style flag");
          if (!flag.ok()) {
            return flag.status();
          }
          style.flags.push_back(flag.TakeValue());
        }
      } else {
        return TokenError(Peek(), ErrorCode::kSyntaxError,
                          "unknown style field: " + TokenValue(Peek()));
      }
    }
    output_.document.styles.push_back(std::move(style));
    ++count;
    SkipLine();
  }
  SourceLocation end = Peek().location;
  status = Expect(TokenKind::kRBrace, "}");
  if (!status.ok()) {
    return status;
  }
  SkipLine();
  return RecordSectionEnd("styles", end, count);
}

Status Parser::ParsePages() {
  const SourceLocation begin = Peek().location;
  MatchWord("pages");
  auto status = Expect(TokenKind::kLBrace, "pages block");
  if (!status.ok()) {
    return status;
  }
  RecordSectionBegin("pages", begin);
  std::size_t count = 0;
  while (!AtEnd() && !Check(TokenKind::kRBrace)) {
    SkipNewlines();
    if (Check(TokenKind::kRBrace) || AtEnd()) {
      break;
    }
    const SourceLocation loc = Peek().location;
    status = ExpectWord("page");
    if (!status.ok()) {
      return status;
    }
    auto id = ExpectIdentifier("page id");
    if (!id.ok()) {
      return id.status();
    }
    Page page;
    page.id = id.TakeValue();
    page.location = loc;
    while (!AtEnd() && !Check(TokenKind::kNewline) && !Check(TokenKind::kRBrace)) {
      if (MatchWord("width")) {
        auto width = ExpectInt("page width");
        if (!width.ok()) {
          return width.status();
        }
        page.width = width.TakeValue();
      } else if (MatchWord("height")) {
        auto height = ExpectInt("page height");
        if (!height.ok()) {
          return height.status();
        }
        page.height = height.TakeValue();
      } else if (MatchWord("background")) {
        status = ParseColorToken(&page.background);
        if (!status.ok()) {
          return status;
        }
      } else if (MatchWord("label")) {
        auto label = ExpectValue("page label");
        if (!label.ok()) {
          return label.status();
        }
        page.label = TokenValue(label.value());
      } else {
        return TokenError(Peek(), ErrorCode::kSyntaxError,
                          "unknown page field: " + TokenValue(Peek()));
      }
    }
    output_.document.pages.push_back(std::move(page));
    ++count;
    SkipLine();
  }
  SourceLocation end = Peek().location;
  status = Expect(TokenKind::kRBrace, "}");
  if (!status.ok()) {
    return status;
  }
  SkipLine();
  return RecordSectionEnd("pages", end, count);
}

Status Parser::ParseOps() {
  const SourceLocation begin = Peek().location;
  MatchWord("ops");
  auto page_id = ExpectIdentifier("ops page id");
  if (!page_id.ok()) {
    return page_id.status();
  }
  auto status = Expect(TokenKind::kLBrace, "ops block");
  if (!status.ok()) {
    return status;
  }
  RecordSectionBegin("ops:" + page_id.value(), begin);
  ScriptBlock script;
  script.page_id = page_id.TakeValue();
  script.location = begin;
  while (!AtEnd() && !Check(TokenKind::kRBrace)) {
    SkipNewlines();
    if (Check(TokenKind::kRBrace) || AtEnd()) {
      break;
    }
    status = ParseOperation(script.page_id, &script);
    if (!status.ok()) {
      return status;
    }
    SkipLine();
  }
  SourceLocation end = Peek().location;
  status = Expect(TokenKind::kRBrace, "}");
  if (!status.ok()) {
    return status;
  }
  SkipLine();
  const std::size_t count = script.operations.size();
  output_.document.scripts.push_back(std::move(script));
  return RecordSectionEnd("ops:" + output_.document.scripts.back().page_id, end,
                          count);
}

Status Parser::ParseOperation(const std::string& page_id, ScriptBlock* script) {
  if (parsed_operations_ >= options_.max_operations) {
    return TokenError(Peek(), ErrorCode::kInternalLimit,
                      "operation limit exceeded");
  }
  const SourceLocation loc = Peek().location;
  auto opcode = ExpectIdentifier("operation");
  if (!opcode.ok()) {
    return opcode.status();
  }
  Operation op;
  op.page = page_id;
  op.location = loc;
  const std::string name = opcode.TakeValue();
  if (name == "layer") {
    op.code = OpCode::kLayer;
    auto id = ExpectIdentifier("layer id");
    if (!id.ok()) return id.status();
    op.layer = id.TakeValue();
  } else if (name == "close") {
    op.code = OpCode::kCloseLayer;
    auto id = ExpectIdentifier("layer id");
    if (!id.ok()) return id.status();
    op.layer = id.TakeValue();
  } else if (name == "text") {
    op.code = OpCode::kText;
    auto layer = ExpectIdentifier("text layer");
    if (!layer.ok()) return layer.status();
    op.layer = layer.TakeValue();
  } else if (name == "box") {
    op.code = OpCode::kBox;
    auto layer = ExpectIdentifier("box layer");
    if (!layer.ok()) return layer.status();
    op.layer = layer.TakeValue();
  } else if (name == "anchor") {
    op.code = OpCode::kAnchor;
    auto layer = ExpectIdentifier("anchor layer");
    if (!layer.ok()) return layer.status();
    auto id = ExpectIdentifier("anchor id");
    if (!id.ok()) return id.status();
    op.layer = layer.TakeValue();
    op.id = id.TakeValue();
  } else if (name == "defer") {
    op.code = OpCode::kDeferredAnchor;
    auto layer = ExpectIdentifier("deferred anchor layer");
    if (!layer.ok()) return layer.status();
    auto id = ExpectIdentifier("deferred anchor id");
    if (!id.ok()) return id.status();
    op.layer = layer.TakeValue();
    op.id = id.TakeValue();
  } else if (name == "clip") {
    op.code = OpCode::kClip;
    auto layer = ExpectIdentifier("clip layer");
    if (!layer.ok()) return layer.status();
    op.layer = layer.TakeValue();
  } else if (name == "attach") {
    op.code = OpCode::kAttach;
    auto layer = ExpectIdentifier("attachment layer");
    if (!layer.ok()) return layer.status();
    op.layer = layer.TakeValue();
  } else if (name == "push") {
    op.code = OpCode::kPushState;
  } else if (name == "pop") {
    op.code = OpCode::kPopState;
  } else if (name == "commit") {
    op.code = OpCode::kCommit;
  } else if (name == "set") {
    op.code = OpCode::kSet;
  } else if (options_.allow_unknown_operations) {
    op.code = OpCode::kNoop;
    op.id = name;
  } else {
    return TokenError(Peek(), ErrorCode::kSyntaxError,
                      "unknown operation: " + name);
  }
  auto status = ParseOperationFields(&op);
  if (!status.ok()) {
    return status;
  }
  script->operations.push_back(std::move(op));
  ++parsed_operations_;
  return Status::Ok();
}

Status Parser::ParseOperationFields(Operation* op) {
  while (!AtEnd() && !Check(TokenKind::kNewline) && !Check(TokenKind::kRBrace)) {
    if (MatchWord("z")) {
      auto v = ExpectInt("z");
      if (!v.ok()) return v.status();
      op->z = v.TakeValue();
    } else if (MatchWord("blend")) {
      auto v = ExpectIdentifier("blend mode");
      if (!v.ok()) return v.status();
      op->blend = v.TakeValue();
    } else if (MatchWord("opacity")) {
      auto v = ExpectInt("opacity");
      if (!v.ok()) return v.status();
      op->opacity = v.TakeValue();
    } else if (MatchWord("style")) {
      auto v = ExpectIdentifier("style id");
      if (!v.ok()) return v.status();
      op->style = v.TakeValue();
    } else if (MatchWord("at")) {
      auto x = ExpectInt("x");
      if (!x.ok()) return x.status();
      auto y = ExpectInt("y");
      if (!y.ok()) return y.status();
      op->x = x.TakeValue();
      op->y = y.TakeValue();
    } else if (MatchWord("rect")) {
      auto x = ExpectInt("rect x");
      if (!x.ok()) return x.status();
      auto y = ExpectInt("rect y");
      if (!y.ok()) return y.status();
      auto w = ExpectInt("rect width");
      if (!w.ok()) return w.status();
      auto h = ExpectInt("rect height");
      if (!h.ok()) return h.status();
      op->x = x.TakeValue();
      op->y = y.TakeValue();
      op->w = w.TakeValue();
      op->h = h.TakeValue();
    } else if (MatchWord("string")) {
      auto v = ExpectIdentifier("string key");
      if (!v.ok()) return v.status();
      op->string_key = v.TakeValue();
    } else if (MatchWord("from")) {
      auto v = ExpectIdentifier("source anchor");
      if (!v.ok()) return v.status();
      op->from_anchor = v.TakeValue();
    } else if (MatchWord("dx")) {
      auto v = ExpectInt("dx");
      if (!v.ok()) return v.status();
      op->dx = v.TakeValue();
    } else if (MatchWord("dy")) {
      auto v = ExpectInt("dy");
      if (!v.ok()) return v.status();
      op->dy = v.TakeValue();
    } else if (MatchWord("page")) {
      auto v = ExpectIdentifier("target page");
      if (!v.ok()) return v.status();
      op->target_page = v.TakeValue();
    } else if (MatchWord("anchor")) {
      auto v = ExpectIdentifier("target anchor");
      if (!v.ok()) return v.status();
      op->target_anchor = v.TakeValue();
    } else if (Match(TokenKind::kComma)) {
      continue;
    } else {
      auto v = ExpectValue("operation argument");
      if (!v.ok()) {
        return v.status();
      }
      op->args.push_back(TokenValue(v.value()));
    }
  }
  return Status::Ok();
}

Status Parser::ParseXref() {
  const SourceLocation begin = Peek().location;
  MatchWord("xref");
  auto status = Expect(TokenKind::kLBrace, "xref block");
  if (!status.ok()) {
    return status;
  }
  RecordSectionBegin("xref", begin);
  std::size_t count = 0;
  while (!AtEnd() && !Check(TokenKind::kRBrace)) {
    SkipNewlines();
    if (Check(TokenKind::kRBrace) || AtEnd()) {
      break;
    }
    const SourceLocation loc = Peek().location;
    status = ExpectWord("alias");
    if (!status.ok()) {
      return status;
    }
    auto alias = ExpectIdentifier("alias");
    if (!alias.ok()) return alias.status();
    auto page = ExpectIdentifier("alias page");
    if (!page.ok()) return page.status();
    auto anchor = ExpectIdentifier("alias anchor");
    if (!anchor.ok()) return anchor.status();
    Xref xref;
    xref.alias = alias.TakeValue();
    xref.page = page.TakeValue();
    xref.anchor = anchor.TakeValue();
    xref.location = loc;
    output_.document.xrefs.push_back(std::move(xref));
    ++count;
    SkipLine();
  }
  SourceLocation end = Peek().location;
  status = Expect(TokenKind::kRBrace, "}");
  if (!status.ok()) {
    return status;
  }
  SkipLine();
  return RecordSectionEnd("xref", end, count);
}

}  // namespace inkspool

