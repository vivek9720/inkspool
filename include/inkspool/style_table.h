#ifndef INKSPOOL_STYLE_TABLE_H
#define INKSPOOL_STYLE_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/document.h"
#include "inkspool/status.h"

namespace inkspool {

struct ResolvedStyle {
  std::string id;
  std::string font;
  Color color;
  int size = 12;
  int tracking = 0;
  std::vector<std::string> flags;
};

class StyleTable {
 public:
  Status Load(const std::vector<Style>& styles);
  const Style* FindRaw(const std::string& id) const;
  Result<ResolvedStyle> Resolve(const std::string& id) const;
  bool Contains(const std::string& id) const;
  std::vector<std::string> Ids() const;

 private:
  Result<ResolvedStyle> ResolveInternal(const std::string& id,
                                        std::vector<std::string>* stack) const;

  std::unordered_map<std::string, Style> styles_;
  std::vector<std::string> order_;
};

}  // namespace inkspool

#endif  // INKSPOOL_STYLE_TABLE_H

