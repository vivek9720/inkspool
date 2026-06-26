#ifndef INKSPOOL_UTIL_H
#define INKSPOOL_UTIL_H

#include <cstdint>
#include <string>
#include <vector>

#include "inkspool/document.h"
#include "inkspool/status.h"

namespace inkspool {

std::string ToLower(std::string value);
bool EqualsIgnoreCase(const std::string& a, const std::string& b);
bool IsIdentifierLike(const std::string& value);
std::vector<std::string> SplitWords(const std::string& value);
std::string Join(const std::vector<std::string>& values,
                 const std::string& separator);
Result<int> ParseIntStrict(const std::string& value);
Result<Color> ParseColor(const std::string& value);
int ClampInt(int value, int low, int high);
std::string Trim(const std::string& value);

}  // namespace inkspool

#endif  // INKSPOOL_UTIL_H

