#include "vkgs/rendering_task.h"

#include "vkgs/core/rendering_task.h"

namespace vkgs {

RenderingTask::RenderingTask(std::shared_ptr<core::RenderingTask> rendering_task) : rendering_task_(rendering_task) {}

RenderingTask::~RenderingTask() {}

void RenderingTask::Wait() const { rendering_task_->Wait(); }

}  // namespace vkgs
