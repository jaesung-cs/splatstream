#ifndef VKGS_CORE_RENDERING_TASK_H
#define VKGS_CORE_RENDERING_TASK_H

#include <memory>

#include "export_api.h"

#include "vkgs/core/draw_result.h"
#include "vkgs/gpu/queue_task.h"

namespace vkgs {
namespace core {

class VKGS_CORE_API RenderingTask {
 public:
  RenderingTask();
  ~RenderingTask();

  void SetTask(gpu::QueueTask task) { task_ = task; }
  void SetDrawResult(const DrawResult& result) { result_ = result; }

  const auto& draw_result() const noexcept { return result_; }

  void Wait();

 private:
  gpu::QueueTask task_;
  DrawResult result_ = {};
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERING_TASK_H
