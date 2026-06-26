#include "inkspool/format_writer.h"

#include <algorithm>
#include <sstream>

namespace inkspool {

namespace {

void AppendOperation(std::ostringstream* out, const Operation& op) {
  switch (op.code) {
    case OpCode::kNoop:
      *out << "  " << (op.id.empty() ? "noop" : op.id);
      for (const auto& arg : op.args) {
        *out << " " << arg;
      }
      break;
    case OpCode::kLayer:
      *out << "  layer " << op.layer << " z " << op.z << " blend "
           << (op.blend.empty() ? "normal" : op.blend) << " opacity "
           << op.opacity;
      break;
    case OpCode::kCloseLayer:
      *out << "  close " << op.layer;
      break;
    case OpCode::kText:
      *out << "  text " << op.layer << " style " << op.style << " at "
           << op.x << " " << op.y << " string " << op.string_key;
      break;
    case OpCode::kBox:
      *out << "  box " << op.layer << " style " << op.style << " rect "
           << op.x << " " << op.y << " " << op.w << " " << op.h;
      break;
    case OpCode::kAnchor:
      *out << "  anchor " << op.layer << " " << op.id << " at " << op.x
           << " " << op.y;
      break;
    case OpCode::kDeferredAnchor:
      *out << "  defer " << op.layer << " " << op.id << " from "
           << op.from_anchor << " dx " << op.dx << " dy " << op.dy;
      break;
    case OpCode::kClip:
      *out << "  clip " << op.layer << " rect " << op.x << " " << op.y
           << " " << op.w << " " << op.h;
      break;
    case OpCode::kAttach:
      *out << "  attach " << op.layer << " page " << op.target_page
           << " anchor " << op.target_anchor;
      break;
    case OpCode::kPushState:
      *out << "  push";
      break;
    case OpCode::kPopState:
      *out << "  pop";
      break;
    case OpCode::kCommit:
      *out << "  commit";
      break;
    case OpCode::kSet:
      *out << "  set";
      for (const auto& arg : op.args) {
        *out << " " << arg;
      }
      break;
  }
}

}  // namespace

FormatWriter::FormatWriter(WriterOptions options) : options_(options) {}

std::string FormatWriter::Quote(const std::string& value) const {
  std::string out = "\"";
  for (char c : value) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  out.push_back('"');
  return out;
}

std::string FormatWriter::Write(const Document& document) const {
  std::ostringstream out;
  out << "INKSPOOL/1\n";
  if (options_.include_metadata) {
    for (const auto& pair : document.metadata.fields) {
      out << "meta " << pair.first << " " << Quote(pair.second) << "\n";
    }
  }
  out << "strings {\n";
  for (const auto& entry : document.strings) {
    out << "  entry " << entry.key << " = " << Quote(entry.value);
    if (!entry.flags.empty()) {
      out << " flags";
      for (const auto& flag : entry.flags) {
        out << " " << flag;
      }
    }
    out << "\n";
  }
  out << "}\n";
  out << "styles {\n";
  for (const auto& style : document.styles) {
    out << "  style " << style.id << " font " << style.font << " size "
        << style.size << " color " << style.color.ToHex();
    if (!style.inherit.empty()) {
      out << " inherit " << style.inherit;
    }
    if (style.tracking != 0) {
      out << " tracking " << style.tracking;
    }
    if (!style.flags.empty()) {
      out << " flags";
      for (const auto& flag : style.flags) {
        out << " " << flag;
      }
    }
    out << "\n";
  }
  out << "}\n";
  out << "pages {\n";
  for (const auto& page : document.pages) {
    out << "  page " << page.id << " width " << page.width << " height "
        << page.height << " background " << page.background.ToHex();
    if (!page.label.empty()) {
      out << " label " << Quote(page.label);
    }
    out << "\n";
  }
  out << "}\n";
  for (const auto& script : document.scripts) {
    out << "ops " << script.page_id << " {\n";
    for (const auto& op : script.operations) {
      AppendOperation(&out, op);
      out << "\n";
    }
    out << "}\n";
  }
  out << "xref {\n";
  for (const auto& xref : document.xrefs) {
    out << "  alias " << xref.alias << " " << xref.page << " " << xref.anchor
        << "\n";
  }
  out << "}\nEND\n";
  return out.str();
}

}  // namespace inkspool
