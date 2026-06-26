#ifndef INKSPOOL_STATUS_H
#define INKSPOOL_STATUS_H

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace inkspool {

enum class ErrorCode {
  kOk = 0,
  kTruncatedInput,
  kBadMagic,
  kSyntaxError,
  kInvalidNumber,
  kDuplicateId,
  kUnknownReference,
  kRangeError,
  kSemanticError,
  kInternalLimit,
};

class Status {
 public:
  Status();
  Status(ErrorCode code, std::string message, std::size_t offset = 0,
         std::size_t line = 0, std::size_t column = 0);

  static Status Ok();
  static Status Error(ErrorCode code, std::string message,
                      std::size_t offset = 0, std::size_t line = 0,
                      std::size_t column = 0);

  bool ok() const { return code_ == ErrorCode::kOk; }
  ErrorCode code() const { return code_; }
  const std::string& message() const { return message_; }
  std::size_t offset() const { return offset_; }
  std::size_t line() const { return line_; }
  std::size_t column() const { return column_; }
  std::string ToString() const;

 private:
  ErrorCode code_;
  std::string message_;
  std::size_t offset_;
  std::size_t line_;
  std::size_t column_;
};

template <typename T>
class Result {
 public:
  Result(const T& value) : status_(Status::Ok()), value_(value) {}
  Result(T&& value) : status_(Status::Ok()), value_(std::move(value)) {}
  Result(Status status) : status_(std::move(status)), value_(std::nullopt) {}

  bool ok() const { return status_.ok(); }
  const Status& status() const { return status_; }
  T& value() { return *value_; }
  const T& value() const { return *value_; }
  T TakeValue() { return std::move(*value_); }

 private:
  Status status_;
  std::optional<T> value_;
};

const char* ErrorCodeName(ErrorCode code);

}  // namespace inkspool

#endif  // INKSPOOL_STATUS_H

