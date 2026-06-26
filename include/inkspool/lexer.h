#ifndef INKSPOOL_LEXER_H
#define INKSPOOL_LEXER_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "inkspool/byte_view.h"
#include "inkspool/status.h"
#include "inkspool/token.h"

namespace inkspool {

class Lexer {
 public:
  explicit Lexer(ByteView input);
  Result<std::vector<Token>> Tokenize();

 private:
  bool AtEnd() const;
  std::uint8_t Peek(std::size_t ahead = 0) const;
  std::uint8_t Advance();
  SourceLocation CurrentLocation() const;
  void SkipHorizontalSpace();
  void SkipLineComment();
  Token ReadQuoted();
  Token ReadWordOrNumber();
  Token MakeToken(TokenKind kind, std::string text, SourceLocation location,
                  std::int64_t number = 0) const;
  void NoteNewline();

  ByteView input_;
  std::size_t pos_ = 0;
  std::size_t line_ = 1;
  std::size_t column_ = 1;
};

}  // namespace inkspool

#endif  // INKSPOOL_LEXER_H

