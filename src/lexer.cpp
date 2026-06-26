#include "inkspool/lexer.h"

#include <cctype>
#include <limits>

namespace inkspool {

const char* TokenKindName(TokenKind kind) {
  switch (kind) {
    case TokenKind::kIdentifier:
      return "identifier";
    case TokenKind::kNumber:
      return "number";
    case TokenKind::kString:
      return "string";
    case TokenKind::kLBrace:
      return "{";
    case TokenKind::kRBrace:
      return "}";
    case TokenKind::kEqual:
      return "=";
    case TokenKind::kComma:
      return ",";
    case TokenKind::kNewline:
      return "newline";
    case TokenKind::kEndOfFile:
      return "eof";
  }
  return "unknown";
}

Lexer::Lexer(ByteView input) : input_(input) {}

bool Lexer::AtEnd() const { return pos_ >= input_.size(); }

std::uint8_t Lexer::Peek(std::size_t ahead) const {
  const std::size_t index = pos_ + ahead;
  if (index >= input_.size()) {
    return 0;
  }
  return input_[index];
}

std::uint8_t Lexer::Advance() {
  if (AtEnd()) {
    return 0;
  }
  const std::uint8_t byte = input_[pos_++];
  if (byte == '\n') {
    NoteNewline();
  } else {
    ++column_;
  }
  return byte;
}

SourceLocation Lexer::CurrentLocation() const {
  return SourceLocation{pos_, line_, column_};
}

void Lexer::NoteNewline() {
  ++line_;
  column_ = 1;
}

void Lexer::SkipHorizontalSpace() {
  while (!AtEnd()) {
    const std::uint8_t c = Peek();
    if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
      Advance();
      continue;
    }
    if (c == '\r') {
      Advance();
      if (Peek() == '\n') {
        Advance();
      } else {
        NoteNewline();
      }
      continue;
    }
    if (c == '/' && Peek(1) == '/') {
      SkipLineComment();
      continue;
    }
    break;
  }
}

void Lexer::SkipLineComment() {
  while (!AtEnd() && Peek() != '\n') {
    Advance();
  }
}

Token Lexer::MakeToken(TokenKind kind, std::string text,
                       SourceLocation location, std::int64_t number) const {
  return Token{kind, std::move(text), number, location};
}

Token Lexer::ReadQuoted() {
  const SourceLocation start = CurrentLocation();
  Advance();
  std::string text;
  while (!AtEnd()) {
    const std::uint8_t raw = Advance();
    if (raw == '"') {
      return MakeToken(TokenKind::kString, text, start);
    }
    if (raw == '\\' && !AtEnd()) {
      const std::uint8_t escaped = Advance();
      switch (escaped) {
        case 'n':
          text.push_back('\n');
          break;
        case 'r':
          text.push_back('\r');
          break;
        case 't':
          text.push_back('\t');
          break;
        case '"':
          text.push_back('"');
          break;
        case '\\':
          text.push_back('\\');
          break;
        default:
          text.push_back(static_cast<char>(escaped));
          break;
      }
      continue;
    }
    if (raw == '\n') {
      return MakeToken(TokenKind::kString, text, start);
    }
    text.push_back(static_cast<char>(raw));
    if (text.size() > (1u << 20)) {
      return MakeToken(TokenKind::kString, text, start);
    }
  }
  return MakeToken(TokenKind::kString, text, start);
}

Token Lexer::ReadWordOrNumber() {
  const SourceLocation start = CurrentLocation();
  std::string text;
  while (!AtEnd()) {
    const std::uint8_t raw = Peek();
    if (raw == ' ' || raw == '\t' || raw == '\n' || raw == '\r' ||
        raw == '\v' || raw == '\f' || raw == '{' || raw == '}' ||
        raw == '=' || raw == ',' || raw == '"') {
      break;
    }
    if (raw == '/' && Peek(1) == '/') {
      break;
    }
    text.push_back(static_cast<char>(Advance()));
    if (text.size() > 4096) {
      break;
    }
  }

  bool maybe_number = !text.empty();
  std::size_t start_digit = 0;
  if (text.size() > 1 && (text[0] == '-' || text[0] == '+')) {
    start_digit = 1;
  }
  for (std::size_t i = start_digit; i < text.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
      maybe_number = false;
      break;
    }
  }
  if (maybe_number && start_digit < text.size()) {
    std::int64_t number = 0;
    bool negative = text[0] == '-';
    for (std::size_t i = start_digit; i < text.size(); ++i) {
      const int digit = text[i] - '0';
      if (number > (std::numeric_limits<std::int64_t>::max() - digit) / 10) {
        return MakeToken(TokenKind::kIdentifier, text, start);
      }
      number = number * 10 + digit;
    }
    if (negative) {
      number = -number;
    }
    return MakeToken(TokenKind::kNumber, text, start, number);
  }
  return MakeToken(TokenKind::kIdentifier, text, start);
}

Result<std::vector<Token>> Lexer::Tokenize() {
  std::vector<Token> tokens;
  tokens.reserve(256);
  while (!AtEnd()) {
    SkipHorizontalSpace();
    if (AtEnd()) {
      break;
    }
    const SourceLocation location = CurrentLocation();
    const std::uint8_t c = Peek();
    if (c == '\n') {
      Advance();
      tokens.push_back(MakeToken(TokenKind::kNewline, "\n", location));
      continue;
    }
    if (c == '{') {
      Advance();
      tokens.push_back(MakeToken(TokenKind::kLBrace, "{", location));
      continue;
    }
    if (c == '}') {
      Advance();
      tokens.push_back(MakeToken(TokenKind::kRBrace, "}", location));
      continue;
    }
    if (c == '=') {
      Advance();
      tokens.push_back(MakeToken(TokenKind::kEqual, "=", location));
      continue;
    }
    if (c == ',') {
      Advance();
      tokens.push_back(MakeToken(TokenKind::kComma, ",", location));
      continue;
    }
    if (c == '"') {
      tokens.push_back(ReadQuoted());
      continue;
    }
    tokens.push_back(ReadWordOrNumber());
    if (tokens.size() > 1000000) {
      return Status::Error(ErrorCode::kInternalLimit, "too many tokens",
                           location.offset, location.line, location.column);
    }
  }
  tokens.push_back(MakeToken(TokenKind::kEndOfFile, "", CurrentLocation()));
  return tokens;
}

}  // namespace inkspool

