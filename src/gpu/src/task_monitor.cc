#include "vkgs/gpu/task_monitor.h"

#include "vkgs/gpu/queue_task.h"

namespace vkgs {
namespace gpu {

TaskMonitorImpl::TaskMonitorImpl() = default;

TaskMonitorImpl::~TaskMonitorImpl() { FinishAllTasks(); }

void TaskMonitorImpl::FinishAllTasks() {
  for (auto task : tasks_) task->Wait();
  tasks_.clear();
}

QueueTask TaskMonitorImpl::Add(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                               std::function<void()> callback) {
  gc();

  auto queue_task = QueueTask::Create(fence, command, std::move(objects), callback);
  tasks_.push_back(queue_task);
  return queue_task;
}

void TaskMonitorImpl::gc() {
  constexpr int LOOP = 5;

  for (int i = 0; i < LOOP && i < tasks_.size(); ++i) {
    rotation_index_ = (rotation_index_ + 1) % tasks_.size();

    if (tasks_[rotation_index_]->IsDone()) {
      std::swap(tasks_[rotation_index_], tasks_.back());
      tasks_.pop_back();
    }
  }
}

}  // namespace gpu
}  // namespace vkgs
