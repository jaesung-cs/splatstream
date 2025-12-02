#ifndef VKGS_RENDERING_TASK_H
#define VKGS_RENDERING_TASK_H

#include <memory>
#include <vector>

#include "vkgs/export_api.h"

#include "vkgs/draw_result.h"

namespace vkgs {
namespace core {
class RenderingTask;
}

class VKGS_API RenderingTask {
 public:
  explicit RenderingTask(core::RenderingTask task);
  ~RenderingTask();

  void Wait();

  const DrawResult& draw_result() const;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERING_TASK_H
