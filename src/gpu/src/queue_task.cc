#include "vkgs/gpu/queue_task.h"

#include "fence.h"

namespace vkgs {
namespace gpu {

QueueTask::QueueTask(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
                     std::vector<std::shared_ptr<Object>> objects, std::function<void()> callback)
    : fence_(fence), command_(command), objects_(std::move(objects)), callback_(callback) {}

QueueTask::~QueueTask() { Wait(); }

bool QueueTask::IsDone() {
  if (fence_->IsSignaled()) {
    if (callback_) {
      callback_();
      callback_ = {};
    }
    return true;
  }
  return false;
}

void QueueTask::Wait() {
  fence_->Wait();

  if (callback_) {
    callback_();
    callback_ = {};
  }
}

}  // namespace gpu
}  // namespace vkgs
