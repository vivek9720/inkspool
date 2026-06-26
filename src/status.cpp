#include "inkspool/status.h"

#include <utility>

namespace inkspool {

Status::Status()
    : code_(ErrorCode::kOk), message_("ok"), offset_(0), line_(0), column_(0) {}

Status::Status(ErrorCode code, std::string message, std::size_t offset,
               std::size_t line, std::size_t column)
    : code_(code),
      message_(std::move(message)),
      offset_(offset),
      line_(line),
      column_(column) {}

Status Status::Ok() { return Status(); }

Status Status::Error(ErrorCode code, std::string message, std::size_t offset,
                     std::size_t line, std::size_t column) {
  return Status(code, std::move(message), offset, line, column);
}

std::string Status::ToString() const {
  if (ok()) {
    return "ok";
  }
  std::ostringstream out;
  out << ErrorCodeName(code_) << ": " << message_;
  if (line_ != 0) {
    out << " at " << line_ << ":" << column_;
  } else if (offset_ != 0) {
    out << " at byte " << offset_;
  }
  return out.str();
}

const char* ErrorCodeName(ErrorCode code) {
  switch (code) {
    case ErrorCode::kOk:
      return "ok";
    case ErrorCode::kTruncatedInput:
      return "truncated-input";
    case ErrorCode::kBadMagic:
      return "bad-magic";
    case ErrorCode::kSyntaxError:
      return "syntax-error";
    case ErrorCode::kInvalidNumber:
      return "invalid-number";
    case ErrorCode::kDuplicateId:
      return "duplicate-id";
    case ErrorCode::kUnknownReference:
      return "unknown-reference";
    case ErrorCode::kRangeError:
      return "range-error";
    case ErrorCode::kSemanticError:
      return "semantic-error";
    case ErrorCode::kInternalLimit:
      return "internal-limit";
  }
  return "unknown";
}

}  // namespace inkspool

