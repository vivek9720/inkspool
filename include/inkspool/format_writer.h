#ifndef INKSPOOL_FORMAT_WRITER_H
#define INKSPOOL_FORMAT_WRITER_H

#include <string>

#include "inkspool/document.h"

namespace inkspool {

struct WriterOptions {
  bool include_metadata = true;
  bool include_comments = false;
  bool stable_sort = false;
};

class FormatWriter {
 public:
  explicit FormatWriter(WriterOptions options = {});
  std::string Write(const Document& document) const;

 private:
  std::string Quote(const std::string& value) const;
  WriterOptions options_;
};

}  // namespace inkspool

#endif  // INKSPOOL_FORMAT_WRITER_H

