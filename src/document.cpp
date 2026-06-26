#include "inkspool/document.h"

#include <iomanip>
#include <sstream>

namespace inkspool {

std::string Color::ToHex() const {
  std::ostringstream out;
  out << '#'
      << std::hex << std::setfill('0') << std::setw(2)
      << static_cast<int>(r)
      << std::setw(2) << static_cast<int>(g)
      << std::setw(2) << static_cast<int>(b);
  if (a != 255) {
    out << std::setw(2) << static_cast<int>(a);
  }
  return out.str();
}

void Document::Clear() {
  metadata.fields.clear();
  strings.clear();
  styles.clear();
  pages.clear();
  scripts.clear();
  xrefs.clear();
}

const Page* Document::FindPage(const std::string& id) const {
  for (const auto& page : pages) {
    if (page.id == id) {
      return &page;
    }
  }
  return nullptr;
}

const Style* Document::FindStyle(const std::string& id) const {
  for (const auto& style : styles) {
    if (style.id == id) {
      return &style;
    }
  }
  return nullptr;
}

const StringEntry* Document::FindString(const std::string& key) const {
  for (const auto& entry : strings) {
    if (entry.key == key) {
      return &entry;
    }
  }
  return nullptr;
}

const ScriptBlock* Document::FindScript(const std::string& page_id) const {
  for (const auto& script : scripts) {
    if (script.page_id == page_id) {
      return &script;
    }
  }
  return nullptr;
}

std::size_t Document::OperationCount() const {
  std::size_t total = 0;
  for (const auto& script : scripts) {
    total += script.operations.size();
  }
  return total;
}

const char* OpCodeName(OpCode code) {
  switch (code) {
    case OpCode::kNoop:
      return "noop";
    case OpCode::kLayer:
      return "layer";
    case OpCode::kCloseLayer:
      return "close";
    case OpCode::kText:
      return "text";
    case OpCode::kBox:
      return "box";
    case OpCode::kAnchor:
      return "anchor";
    case OpCode::kDeferredAnchor:
      return "defer";
    case OpCode::kClip:
      return "clip";
    case OpCode::kAttach:
      return "attach";
    case OpCode::kPushState:
      return "push";
    case OpCode::kPopState:
      return "pop";
    case OpCode::kCommit:
      return "commit";
    case OpCode::kSet:
      return "set";
  }
  return "unknown";
}

}  // namespace inkspool

