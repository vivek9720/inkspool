#include "inkspool/checksum.h"

#include <iomanip>
#include <sstream>

namespace inkspool {

std::uint64_t Fnv1a64(ByteView data) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::size_t i = 0; i < data.size(); ++i) {
    hash ^= data[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::uint64_t Fnv1a64(const std::string& data) {
  return Fnv1a64(ByteView(reinterpret_cast<const std::uint8_t*>(data.data()),
                          data.size()));
}

std::string Hex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

std::string StableDocumentDigest(const std::string& normalized) {
  return Hex64(Fnv1a64(normalized));
}

}  // namespace inkspool

