#ifndef INKSPOOL_BYTE_VIEW_H
#define INKSPOOL_BYTE_VIEW_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace inkspool {

class ByteView {
 public:
  ByteView() = default;
  ByteView(const std::uint8_t* data, std::size_t size)
      : data_(data), size_(size) {}

  const std::uint8_t* data() const { return data_; }
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  std::uint8_t operator[](std::size_t index) const { return data_[index]; }
  ByteView Subview(std::size_t offset, std::size_t length) const;
  std::string ToLossyString(std::size_t max_bytes = 1 << 20) const;
  std::vector<std::uint8_t> Copy() const;

 private:
  const std::uint8_t* data_ = nullptr;
  std::size_t size_ = 0;
};

}  // namespace inkspool

#endif  // INKSPOOL_BYTE_VIEW_H

