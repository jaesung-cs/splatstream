#include "vkgs/core/rendering_task.h"

#include "vkgs/gpu/task.h"

namespace vkgs {
namespace core {

RenderingTask::RenderingTask(std::shared_ptr<gpu::Task> task) : task_(task) {}

RenderingTask::~RenderingTask() {}

void RenderingTask::Wait() {
  if (task_) {
    task_->Wait();
    task_ = nullptr;
  }
}

}  // namespace core
}  // namespace vkgs
