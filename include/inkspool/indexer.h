#ifndef INKSPOOL_INDEXER_H
#define INKSPOOL_INDEXER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/document.h"
#include "inkspool/page_script.h"
#include "inkspool/status.h"
#include "inkspool/string_pool.h"
#include "inkspool/style_table.h"

namespace inkspool {

struct AnchorDeclaration {
  std::string page;
  std::string layer;
  std::string id;
  int x = 0;
  int y = 0;
  SourceLocation location;
};

struct XrefResolution {
  std::string alias;
  std::string page;
  std::string anchor;
  bool found = false;
};

class DocumentIndex {
 public:
  Status Build(const Document& document);
  void Clear();

  const StringPool& strings() const { return strings_; }
  const StyleTable& styles() const { return styles_; }
  const PageScriptMap& scripts() const { return scripts_; }
  const Page* FindPage(const std::string& id) const;
  const ScriptBlock* FindScript(const std::string& page_id) const;
  const AnchorDeclaration* FindAnchor(const std::string& page,
                                      const std::string& id) const;
  XrefResolution ResolveAlias(const std::string& alias) const;
  std::vector<std::string> PageIds() const;
  std::vector<std::string> AnchorIds(const std::string& page) const;
  std::string Summary() const;

 private:
  StringPool strings_;
  StyleTable styles_;
  PageScriptMap scripts_;
  std::unordered_map<std::string, Page> pages_;
  std::vector<std::string> page_order_;
  std::unordered_map<std::string, AnchorDeclaration> anchors_;
  std::unordered_map<std::string, Xref> aliases_;
};

std::string AnchorKey(const std::string& page, const std::string& anchor);

}  // namespace inkspool

#endif  // INKSPOOL_INDEXER_H

