#ifndef INKSPOOL_REPLAY_ENGINE_H
#define INKSPOOL_REPLAY_ENGINE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "inkspool/document.h"
#include "inkspool/indexer.h"
#include "inkspool/render_plan.h"
#include "inkspool/status.h"

namespace inkspool {

struct ReplayOptions {
  std::size_t max_operations = 8192;
  bool collect_draw_items = true;
  bool keep_debug_trail = false;
};

struct ReplayTrace {
  std::vector<std::string> events;
  void Add(const std::string& event);
};

class ReplayEngine {
 public:
  explicit ReplayEngine(ReplayOptions options = {});
  Result<RenderPlan> ReplayPage(const Document& document,
                                const DocumentIndex& index,
                                const std::string& page_id,
                                ReplayTrace* trace = nullptr) const;

 private:
  struct LayerFrame;
  struct DeferredAnchor;
  struct ReplayState;

  Status ApplyOperation(const Operation& op, const Document& document,
                        const DocumentIndex& index, ReplayState* state,
                        ReplayTrace* trace) const;
  Status ResolveDeferredAnchors(ReplayState* state, ReplayTrace* trace) const;
  static Rect NormalizeRect(int x, int y, int w, int h);

  ReplayOptions options_;
};

}  // namespace inkspool

#endif  // INKSPOOL_REPLAY_ENGINE_H

