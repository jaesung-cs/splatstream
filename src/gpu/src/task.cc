#include "vkgs/gpu/task.h"

#include "vkgs/gpu/fence.h"

namespace vkgs {
namespace gpu {

Task::Task(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
           std::function<void(VkCommandBuffer)> task_callback, std::function<void()> callback)
    : fence_(fence), command_(command), task_callback_(task_callback), callback_(callback) {}

Task::~Task() { Wait(); }

bool Task::IsDone() {
  if (fence_->IsSignaled()) {
    task_callback_ = {};

    if (callback_) {
      callback_();
      callback_ = {};
    }
    return true;
  }
  return false;
}

void Task::Wait() {
  fence_->Wait();
  task_callback_ = {};

  if (callback_) {
    callback_();
    callback_ = {};
  }
}

}  // namespace gpu
}  // namespace vkgs
