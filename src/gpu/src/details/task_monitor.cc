#include "details/task_monitor.h"

#include "vkgs/gpu/queue_task.h"

#include "details/fence.h"
#include "details/command.h"

namespace vkgs {
namespace gpu {

class TaskMonitorImpl {
 public:
  void __del__() { FinishAllTasks(); }

  void FinishAllTasks() {
    for (auto task : tasks_) task.Wait();
    tasks_.clear();
  }

  QueueTask Add(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                std::function<void()> callback) {
    gc();

    auto queue_task = QueueTask::Create(fence, command, std::move(objects), callback);
    tasks_.push_back(queue_task);
    return queue_task;
  }

 private:
  void gc() {
    constexpr int LOOP = 5;

    for (int i = 0; i < LOOP && i < tasks_.size(); ++i) {
      rotation_index_ = (rotation_index_ + 1) % tasks_.size();

      if (tasks_[rotation_index_].IsDone()) {
        std::swap(tasks_[rotation_index_], tasks_.back());
        tasks_.pop_back();
      }
    }
  }

  std::vector<QueueTask> tasks_;
  int rotation_index_ = 0;
};

TaskMonitor TaskMonitor::Create() { return Make<TaskMonitorImpl>(); }

void TaskMonitor::FinishAllTasks() { impl_->FinishAllTasks(); }
QueueTask TaskMonitor::Add(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                           std::function<void()> callback) {
  return impl_->Add(fence, command, std::move(objects), callback);
}

}  // namespace gpu
}  // namespace vkgs
