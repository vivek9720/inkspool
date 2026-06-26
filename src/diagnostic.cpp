#include "inkspool/diagnostic.h"

#include <sstream>
#include <utility>

namespace inkspool {

void DiagnosticSink::Add(Diagnostic diagnostic) {
  if (diagnostic.warning) {
    ++warning_count_;
  } else {
    ++error_count_;
  }
  diagnostics_.push_back(std::move(diagnostic));
}

void DiagnosticSink::AddError(ErrorCode code, const std::string& message,
                              SourceLocation location) {
  Add(Diagnostic{code, message, location, false});
}

void DiagnosticSink::AddWarning(ErrorCode code, const std::string& message,
                                SourceLocation location) {
  Add(Diagnostic{code, message, location, true});
}

bool DiagnosticSink::HasErrors() const { return error_count_ != 0; }

std::string DiagnosticSink::FormatAll() const {
  std::ostringstream out;
  for (const auto& diagnostic : diagnostics_) {
    out << (diagnostic.warning ? "warning" : "error") << " "
        << ErrorCodeName(diagnostic.code) << " at "
        << diagnostic.location.line << ":" << diagnostic.location.column
        << ": " << diagnostic.message << "\n";
  }
  return out.str();
}

Status StatusFromDiagnostic(const Diagnostic& diagnostic) {
  return Status::Error(diagnostic.code, diagnostic.message,
                       diagnostic.location.offset, diagnostic.location.line,
                       diagnostic.location.column);
}

}  // namespace inkspool

