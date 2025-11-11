#include "vkgs/core/rendering_task.h"

#include "vkgs/gpu/task.h"

namespace vkgs {
namespace core {

RenderingTask::RenderingTask() = default;

RenderingTask::~RenderingTask() = default;

void RenderingTask::SetTask(std::shared_ptr<gpu::Task> task) { task_ = task; }

void RenderingTask::SetDrawResult(const DrawResult& result) { result_ = result; }

void RenderingTask::Wait() {
  if (task_) {
    task_->Wait();
    task_ = nullptr;
  }
}

}  // namespace core
}  // namespace vkgs
