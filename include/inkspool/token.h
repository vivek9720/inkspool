#ifndef INKSPOOL_TOKEN_H
#define INKSPOOL_TOKEN_H

#include <cstdint>
#include <string>

#include "inkspool/diagnostic.h"

namespace inkspool {

enum class TokenKind {
  kIdentifier,
  kNumber,
  kString,
  kLBrace,
  kRBrace,
  kEqual,
  kComma,
  kNewline,
  kEndOfFile,
};

struct Token {
  TokenKind kind = TokenKind::kEndOfFile;
  std::string text;
  std::int64_t number = 0;
  SourceLocation location;
};

const char* TokenKindName(TokenKind kind);

}  // namespace inkspool

#endif  // INKSPOOL_TOKEN_H

