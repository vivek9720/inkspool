#ifndef INKSPOOL_DOCUMENT_H
#define INKSPOOL_DOCUMENT_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/diagnostic.h"

namespace inkspool {

struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;

  std::string ToHex() const;
};

struct Metadata {
  std::map<std::string, std::string> fields;
  SourceLocation location;
};

struct StringEntry {
  std::string key;
  std::string value;
  std::vector<std::string> flags;
  SourceLocation location;
};

struct Style {
  std::string id;
  std::string font;
  std::string inherit;
  Color color;
  int size = 12;
  int tracking = 0;
  std::vector<std::string> flags;
  SourceLocation location;
};

struct Page {
  std::string id;
  int width = 0;
  int height = 0;
  Color background;
  std::string label;
  SourceLocation location;
};

enum class OpCode {
  kNoop,
  kLayer,
  kCloseLayer,
  kText,
  kBox,
  kAnchor,
  kDeferredAnchor,
  kClip,
  kAttach,
  kPushState,
  kPopState,
  kCommit,
  kSet,
};

struct Operation {
  OpCode code = OpCode::kNoop;
  std::string page;
  std::string layer;
  std::string id;
  std::string style;
  std::string string_key;
  std::string from_anchor;
  std::string target_page;
  std::string target_anchor;
  std::string blend;
  std::vector<std::string> args;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int z = 0;
  int dx = 0;
  int dy = 0;
  int opacity = 255;
  SourceLocation location;
};

struct ScriptBlock {
  std::string page_id;
  std::vector<Operation> operations;
  SourceLocation location;
};

struct Xref {
  std::string alias;
  std::string page;
  std::string anchor;
  SourceLocation location;
};

struct Document {
  Metadata metadata;
  std::vector<StringEntry> strings;
  std::vector<Style> styles;
  std::vector<Page> pages;
  std::vector<ScriptBlock> scripts;
  std::vector<Xref> xrefs;

  void Clear();
  const Page* FindPage(const std::string& id) const;
  const Style* FindStyle(const std::string& id) const;
  const StringEntry* FindString(const std::string& key) const;
  const ScriptBlock* FindScript(const std::string& page_id) const;
  std::size_t OperationCount() const;
};

const char* OpCodeName(OpCode code);

}  // namespace inkspool

#endif  // INKSPOOL_DOCUMENT_H

