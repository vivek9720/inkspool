#ifndef INKSPOOL_RENDER_PLAN_H
#define INKSPOOL_RENDER_PLAN_H

#include <string>
#include <vector>

#include "inkspool/document.h"

namespace inkspool {

struct Rect {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

struct DrawItem {
  std::string layer;
  std::string kind;
  std::string text;
  std::string style;
  Rect bounds;
  int z = 0;
};

struct AnchorPlacement {
  std::string id;
  std::string layer;
  int x = 0;
  int y = 0;
};

struct RenderPlan {
  std::string page;
  int width = 0;
  int height = 0;
  Color background;
  std::vector<DrawItem> items;
  std::vector<AnchorPlacement> anchors;
  std::vector<std::string> attachments;

  std::string Summary() const;
};

}  // namespace inkspool

#endif  // INKSPOOL_RENDER_PLAN_H

