#ifndef INKSPOOL_VALIDATOR_H
#define INKSPOOL_VALIDATOR_H

#include <string>
#include <unordered_set>

#include "inkspool/diagnostic.h"
#include "inkspool/document.h"
#include "inkspool/status.h"

namespace inkspool {

struct ValidationOptions {
  std::size_t max_page_area = 20000u * 20000u;
  std::size_t max_operations = 8192;
  bool require_scripts_for_pages = false;
};

struct ValidationReport {
  DiagnosticSink diagnostics;
  std::size_t page_count = 0;
  std::size_t style_count = 0;
  std::size_t string_count = 0;
  std::size_t operation_count = 0;
  bool ok() const { return !diagnostics.HasErrors(); }
};

class Validator {
 public:
  explicit Validator(ValidationOptions options = {});
  ValidationReport Validate(const Document& document) const;

 private:
  void CheckStrings(const Document& document, ValidationReport* report) const;
  void CheckStyles(const Document& document, ValidationReport* report) const;
  void CheckPages(const Document& document, ValidationReport* report) const;
  void CheckScripts(const Document& document, ValidationReport* report) const;
  void CheckXrefs(const Document& document, ValidationReport* report) const;

  ValidationOptions options_;
};

Status ValidateDocument(const Document& document,
                        ValidationOptions options = {});

}  // namespace inkspool

#endif  // INKSPOOL_VALIDATOR_H

