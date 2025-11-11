#include "vkgs/rendering_task.h"

#include "vkgs/core/rendering_task.h"

namespace vkgs {

RenderingTask::RenderingTask(std::shared_ptr<core::RenderingTask> task) : task_(task) {}

RenderingTask::~RenderingTask() {}

void RenderingTask::Wait() {
  task_->Wait();

  auto result = task_->draw_result();
  result_.compute_timestamp = result.compute_timestamp;
  result_.graphics_timestamp = result.graphics_timestamp;
  result_.transfer_timestamp = result.transfer_timestamp;
}

}  // namespace vkgs
