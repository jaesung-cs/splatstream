#include "vkgs/gpu/task.h"

#include "vkgs/gpu/fence.h"

namespace vkgs {
namespace gpu {

Task::Task(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects, std::function<void()> callback)
    : fence_(std::move(fence)), objects_(std::move(objects)), callback_(std::move(callback)) {}

Task::~Task() { Wait(); }

bool Task::IsDone() {
  if (fence_->IsSignaled()) {
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

  if (callback_) {
    callback_();
    callback_ = {};
  }
}

}  // namespace gpu
}  // namespace vkgs
