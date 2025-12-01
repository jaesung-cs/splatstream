#include "vkgs/core/rendering_task.h"

namespace vkgs {
namespace core {

RenderingTaskImpl::RenderingTaskImpl() = default;

RenderingTaskImpl::~RenderingTaskImpl() = default;

void RenderingTaskImpl::Wait() {
  if (task_) {
    task_->Wait();
    task_.reset();
  }
}

}  // namespace core
}  // namespace vkgs
