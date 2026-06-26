#ifndef INKSPOOL_CHECKSUM_H
#define INKSPOOL_CHECKSUM_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "inkspool/byte_view.h"

namespace inkspool {

std::uint64_t Fnv1a64(ByteView data);
std::uint64_t Fnv1a64(const std::string& data);
std::string Hex64(std::uint64_t value);
std::string StableDocumentDigest(const std::string& normalized);

}  // namespace inkspool

#endif  // INKSPOOL_CHECKSUM_H

