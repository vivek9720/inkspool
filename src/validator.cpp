#include "inkspool/validator.h"

#include <unordered_map>

namespace inkspool {

Validator::Validator(ValidationOptions options) : options_(options) {}

ValidationReport Validator::Validate(const Document& document) const {
  ValidationReport report;
  report.page_count = document.pages.size();
  report.style_count = document.styles.size();
  report.string_count = document.strings.size();
  report.operation_count = document.OperationCount();
  CheckStrings(document, &report);
  CheckStyles(document, &report);
  CheckPages(document, &report);
  CheckScripts(document, &report);
  CheckXrefs(document, &report);
  return report;
}

void Validator::CheckStrings(const Document& document,
                             ValidationReport* report) const {
  std::unordered_set<std::string> seen;
  for (const auto& entry : document.strings) {
    if (entry.key.empty()) {
      report->diagnostics.AddError(ErrorCode::kSemanticError,
                                   "empty string key", entry.location);
    }
    if (!seen.insert(entry.key).second) {
      report->diagnostics.AddError(ErrorCode::kDuplicateId,
                                   "duplicate string key: " + entry.key,
                                   entry.location);
    }
    if (entry.value.size() > 65536) {
      report->diagnostics.AddError(ErrorCode::kInternalLimit,
                                   "string entry too large", entry.location);
    }
  }
}

void Validator::CheckStyles(const Document& document,
                            ValidationReport* report) const {
  std::unordered_set<std::string> seen;
  for (const auto& style : document.styles) {
    if (style.id.empty()) {
      report->diagnostics.AddError(ErrorCode::kSemanticError,
                                   "empty style id", style.location);
    }
    if (!seen.insert(style.id).second) {
      report->diagnostics.AddError(ErrorCode::kDuplicateId,
                                   "duplicate style id: " + style.id,
                                   style.location);
    }
    if (!style.inherit.empty() && document.FindStyle(style.inherit) == nullptr) {
      report->diagnostics.AddError(ErrorCode::kUnknownReference,
                                   "unknown parent style: " + style.inherit,
                                   style.location);
    }
    if (style.size <= 0 || style.size > 512) {
      report->diagnostics.AddError(ErrorCode::kRangeError,
                                   "style size out of range", style.location);
    }
    if (style.tracking < -4096 || style.tracking > 4096) {
      report->diagnostics.AddError(ErrorCode::kRangeError,
                                   "tracking out of range", style.location);
    }
  }
}

void Validator::CheckPages(const Document& document,
                           ValidationReport* report) const {
  std::unordered_set<std::string> seen;
  for (const auto& page : document.pages) {
    if (page.id.empty()) {
      report->diagnostics.AddError(ErrorCode::kSemanticError, "empty page id",
                                   page.location);
    }
    if (!seen.insert(page.id).second) {
      report->diagnostics.AddError(ErrorCode::kDuplicateId,
                                   "duplicate page id: " + page.id,
                                   page.location);
    }
    if (page.width <= 0 || page.height <= 0) {
      report->diagnostics.AddError(ErrorCode::kRangeError,
                                   "page dimensions must be positive",
                                   page.location);
      continue;
    }
    const auto area = static_cast<std::size_t>(page.width) *
                      static_cast<std::size_t>(page.height);
    if (area > options_.max_page_area) {
      report->diagnostics.AddError(ErrorCode::kRangeError,
                                   "page area exceeds configured limit",
                                   page.location);
    }
  }
}

void Validator::CheckScripts(const Document& document,
                             ValidationReport* report) const {
  if (document.OperationCount() > options_.max_operations) {
    SourceLocation loc;
    if (!document.scripts.empty()) {
      loc = document.scripts.front().location;
    }
    report->diagnostics.AddError(ErrorCode::kInternalLimit,
                                 "too many operations in document", loc);
  }
  std::unordered_set<std::string> scripts_seen;
  for (const auto& script : document.scripts) {
    if (document.FindPage(script.page_id) == nullptr) {
      report->diagnostics.AddError(ErrorCode::kUnknownReference,
                                   "script references unknown page: " +
                                       script.page_id,
                                   script.location);
    }
    if (!scripts_seen.insert(script.page_id).second) {
      report->diagnostics.AddError(ErrorCode::kDuplicateId,
                                   "duplicate script for page: " +
                                       script.page_id,
                                   script.location);
    }
    std::unordered_set<std::string> declared_layers;
    std::unordered_set<std::string> declared_anchors;
    for (const auto& op : script.operations) {
      if (op.code == OpCode::kLayer) {
        declared_layers.insert(op.layer);
      }
      if (!op.style.empty() && document.FindStyle(op.style) == nullptr) {
        report->diagnostics.AddError(ErrorCode::kUnknownReference,
                                     "unknown style in operation: " + op.style,
                                     op.location);
      }
      if (!op.string_key.empty() &&
          document.FindString(op.string_key) == nullptr) {
        report->diagnostics.AddError(
            ErrorCode::kUnknownReference,
            "unknown string in operation: " + op.string_key, op.location);
      }
      if ((op.code == OpCode::kText || op.code == OpCode::kBox ||
           op.code == OpCode::kAnchor || op.code == OpCode::kDeferredAnchor ||
           op.code == OpCode::kClip || op.code == OpCode::kAttach ||
           op.code == OpCode::kCloseLayer) &&
          declared_layers.find(op.layer) == declared_layers.end()) {
        report->diagnostics.AddWarning(
            ErrorCode::kUnknownReference,
            "operation references layer before declaration: " + op.layer,
            op.location);
      }
      if (op.code == OpCode::kAnchor) {
        declared_anchors.insert(op.id);
      }
      if (op.code == OpCode::kDeferredAnchor && !op.from_anchor.empty() &&
          declared_anchors.find(op.from_anchor) == declared_anchors.end()) {
        report->diagnostics.AddWarning(
            ErrorCode::kUnknownReference,
            "deferred anchor source is not declared earlier in the script: " +
                op.from_anchor,
            op.location);
      }
      if (op.opacity < 0 || op.opacity > 255) {
        report->diagnostics.AddError(ErrorCode::kRangeError,
                                     "opacity out of range", op.location);
      }
      if (op.w < 0 || op.h < 0) {
        report->diagnostics.AddError(ErrorCode::kRangeError,
                                     "negative rectangle size", op.location);
      }
    }
  }
  if (options_.require_scripts_for_pages) {
    for (const auto& page : document.pages) {
      if (scripts_seen.find(page.id) == scripts_seen.end()) {
        report->diagnostics.AddError(ErrorCode::kUnknownReference,
                                     "missing script for page: " + page.id,
                                     page.location);
      }
    }
  }
}

void Validator::CheckXrefs(const Document& document,
                           ValidationReport* report) const {
  std::unordered_set<std::string> seen;
  for (const auto& xref : document.xrefs) {
    if (!seen.insert(xref.alias).second) {
      report->diagnostics.AddError(ErrorCode::kDuplicateId,
                                   "duplicate alias: " + xref.alias,
                                   xref.location);
    }
    if (document.FindPage(xref.page) == nullptr) {
      report->diagnostics.AddError(ErrorCode::kUnknownReference,
                                   "alias references unknown page: " +
                                       xref.page,
                                   xref.location);
    }
  }
}

Status ValidateDocument(const Document& document, ValidationOptions options) {
  Validator validator(options);
  auto report = validator.Validate(document);
  if (!report.ok()) {
    return StatusFromDiagnostic(report.diagnostics.diagnostics().front());
  }
  return Status::Ok();
}

}  // namespace inkspool

