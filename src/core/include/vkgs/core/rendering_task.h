#ifndef VKGS_CORE_RENDERING_TASK_H
#define VKGS_CORE_RENDERING_TASK_H

#include <memory>

#include "export_api.h"

#include "vkgs/core/draw_result.h"

namespace vkgs {
namespace gpu {

class Task;

}  // namespace gpu

namespace core {

class VKGS_CORE_API RenderingTask {
 public:
  RenderingTask();
  ~RenderingTask();

  void SetTask(std::shared_ptr<gpu::Task> task);
  void SetDrawResult(const DrawResult& result);

  const auto& draw_result() const noexcept { return result_; }

  void Wait();

 private:
  std::shared_ptr<gpu::Task> task_;
  DrawResult result_ = {};
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERING_TASK_H
