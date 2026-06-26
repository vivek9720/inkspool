#ifndef INKSPOOL_DIAGNOSTIC_H
#define INKSPOOL_DIAGNOSTIC_H

#include <cstddef>
#include <string>
#include <vector>

#include "inkspool/status.h"

namespace inkspool {

struct SourceLocation {
  std::size_t offset = 0;
  std::size_t line = 1;
  std::size_t column = 1;
};

struct Diagnostic {
  ErrorCode code = ErrorCode::kOk;
  std::string message;
  SourceLocation location;
  bool warning = false;
};

class DiagnosticSink {
 public:
  void Add(Diagnostic diagnostic);
  void AddError(ErrorCode code, const std::string& message,
                SourceLocation location);
  void AddWarning(ErrorCode code, const std::string& message,
                  SourceLocation location);
  bool HasErrors() const;
  std::size_t error_count() const { return error_count_; }
  std::size_t warning_count() const { return warning_count_; }
  const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
  std::string FormatAll() const;

 private:
  std::vector<Diagnostic> diagnostics_;
  std::size_t error_count_ = 0;
  std::size_t warning_count_ = 0;
};

Status StatusFromDiagnostic(const Diagnostic& diagnostic);

}  // namespace inkspool

#endif  // INKSPOOL_DIAGNOSTIC_H

