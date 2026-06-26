#include "inkspool/render_plan.h"

#include <sstream>

namespace inkspool {

std::string RenderPlan::Summary() const {
  std::ostringstream out;
  out << "page=" << page << " size=" << width << "x" << height
      << " items=" << items.size() << " anchors=" << anchors.size()
      << " attachments=" << attachments.size();
  return out.str();
}

}  // namespace inkspool

