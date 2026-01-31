#include "vkgs/gpu/queue_task.h"

#include "details/command.h"
#include "details/fence.h"

namespace vkgs {
namespace gpu {

class QueueTaskImpl {
 public:
  void __init__(Fence fence, Command command, std::vector<AnyHandle> objects, std::function<void()> callback) {
    fence_ = fence;
    command_ = command;
    objects_ = std::move(objects);
    callback_ = callback;
  }

  void __del__() { Wait(); }

  bool IsDone() {
    if (fence_.IsSignaled()) {
      if (callback_) {
        callback_();
        callback_ = {};
      }
      return true;
    }
    return false;
  }

  void Wait() {
    fence_.Wait();

    if (callback_) {
      callback_();
      callback_ = {};
    }
  }

 private:
  Fence fence_;
  Command command_;
  std::vector<AnyHandle> objects_;
  std::function<void()> callback_;
};

QueueTask QueueTask::Create(Fence fence, Command command, std::vector<AnyHandle> objects,
                            std::function<void()> callback) {
  return Make<QueueTaskImpl>(fence, command, std::move(objects), callback);
}

bool QueueTask::IsDone() { return impl_->IsDone(); }
void QueueTask::Wait() { impl_->Wait(); }

}  // namespace gpu
}  // namespace vkgs
