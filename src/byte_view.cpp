#include "inkspool/byte_view.h"

#include <algorithm>

namespace inkspool {

ByteView ByteView::Subview(std::size_t offset, std::size_t length) const {
  if (offset > size_) {
    return ByteView();
  }
  const std::size_t remaining = size_ - offset;
  const std::size_t actual = std::min(length, remaining);
  return ByteView(data_ + offset, actual);
}

std::string ByteView::ToLossyString(std::size_t max_bytes) const {
  const std::size_t actual = std::min(max_bytes, size_);
  std::string out;
  out.reserve(actual);
  for (std::size_t i = 0; i < actual; ++i) {
    const std::uint8_t byte = data_[i];
    if (byte == '\0') {
      out.push_back(' ');
    } else if (byte == '\r') {
      if (i + 1 < actual && data_[i + 1] == '\n') {
        continue;
      }
      out.push_back('\n');
    } else {
      out.push_back(static_cast<char>(byte));
    }
  }
  return out;
}

std::vector<std::uint8_t> ByteView::Copy() const {
  if (data_ == nullptr || size_ == 0) {
    return {};
  }
  return std::vector<std::uint8_t>(data_, data_ + size_);
}

}  // namespace inkspool

