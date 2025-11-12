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
  explicit RenderingTask(std::shared_ptr<core::RenderingTask> task);
  ~RenderingTask();

  void Wait();

  const auto& draw_result() const noexcept { return result_; }

 private:
  std::shared_ptr<core::RenderingTask> task_;
  DrawResult result_ = {};
};

}  // namespace vkgs

#endif  // VKGS_RENDERING_TASK_H
