#include "vkgs/core/rendering_task.h"

namespace vkgs {
namespace core {

RenderingTask::RenderingTask() = default;

RenderingTask::~RenderingTask() = default;

void RenderingTask::Wait() {
  if (task_) {
    task_->Wait();
    task_.reset();
  }
}

}  // namespace core
}  // namespace vkgs
