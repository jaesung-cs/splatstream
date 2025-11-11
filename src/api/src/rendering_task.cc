#include "vkgs/rendering_task.h"

#include "vkgs/core/rendering_task.h"

namespace vkgs {

RenderingTask::RenderingTask(std::shared_ptr<core::RenderingTask> task) : task_(task) {}

RenderingTask::~RenderingTask() {}

void RenderingTask::Wait() const { task_->Wait(); }

}  // namespace vkgs
