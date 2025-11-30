#include "vkgs/gpu/queue_task.h"

#include "vkgs/gpu/fence.h"

namespace vkgs {
namespace gpu {

QueueTaskImpl::QueueTaskImpl(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                             std::function<void()> callback)
    : fence_(fence), command_(command), objects_(std::move(objects)), callback_(callback) {}

QueueTaskImpl::~QueueTaskImpl() { Wait(); }

bool QueueTaskImpl::IsDone() {
  if (fence_->IsSignaled()) {
    if (callback_) {
      callback_();
      callback_ = {};
    }
    return true;
  }
  return false;
}

void QueueTaskImpl::Wait() {
  fence_->Wait();

  if (callback_) {
    callback_();
    callback_ = {};
  }
}

}  // namespace gpu
}  // namespace vkgs
