#include "inkspool/util.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace inkspool {

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
  return ToLower(a) == ToLower(b);
}

bool IsIdentifierLike(const std::string& value) {
  if (value.empty()) {
    return false;
  }
  const auto first = static_cast<unsigned char>(value.front());
  if (!(std::isalpha(first) || first == '_' || first == '.' || first == '/')) {
    return false;
  }
  for (unsigned char c : value) {
    if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == '/' ||
          c == ':')) {
      return false;
    }
  }
  return true;
}

std::vector<std::string> SplitWords(const std::string& value) {
  std::istringstream in(value);
  std::vector<std::string> words;
  std::string word;
  while (in >> word) {
    words.push_back(word);
  }
  return words;
}

std::string Join(const std::vector<std::string>& values,
                 const std::string& separator) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << separator;
    }
    out << values[i];
  }
  return out.str();
}

Result<int> ParseIntStrict(const std::string& value) {
  if (value.empty()) {
    return Status::Error(ErrorCode::kInvalidNumber, "empty integer literal");
  }
  std::size_t consumed = 0;
  try {
    const long parsed = std::stol(value, &consumed, 10);
    if (consumed != value.size()) {
      return Status::Error(ErrorCode::kInvalidNumber,
                           "trailing characters in integer literal");
    }
    if (parsed < -2147483647L - 1L || parsed > 2147483647L) {
      return Status::Error(ErrorCode::kRangeError, "integer out of range");
    }
    return static_cast<int>(parsed);
  } catch (const std::exception&) {
    return Status::Error(ErrorCode::kInvalidNumber,
                         "invalid integer literal");
  }
}

static int HexDigit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

Result<Color> ParseColor(const std::string& value) {
  if (value.size() != 7 && value.size() != 9) {
    return Status::Error(ErrorCode::kSyntaxError,
                         "color must be #RRGGBB or #RRGGBBAA");
  }
  if (value[0] != '#') {
    return Status::Error(ErrorCode::kSyntaxError, "color must start with #");
  }
  std::uint8_t bytes[4] = {0, 0, 0, 255};
  for (std::size_t i = 1; i < value.size(); i += 2) {
    const int hi = HexDigit(value[i]);
    const int lo = HexDigit(value[i + 1]);
    if (hi < 0 || lo < 0) {
      return Status::Error(ErrorCode::kSyntaxError, "invalid color digit");
    }
    bytes[(i - 1) / 2] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  Color color;
  color.r = bytes[0];
  color.g = bytes[1];
  color.b = bytes[2];
  color.a = bytes[3];
  return color;
}

int ClampInt(int value, int low, int high) {
  return std::max(low, std::min(value, high));
}

std::string Trim(const std::string& value) {
  std::size_t begin = 0;
  while (begin < value.size() &&
         std::isspace(static_cast<unsigned char>(value[begin]))) {
    ++begin;
  }
  std::size_t end = value.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(begin, end - begin);
}

}  // namespace inkspool

