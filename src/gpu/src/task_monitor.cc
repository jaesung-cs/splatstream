#include "task_monitor.h"

#include "vkgs/gpu/task.h"

namespace vkgs {
namespace gpu {

TaskMonitor::TaskMonitor() = default;

TaskMonitor::~TaskMonitor() = default;

std::shared_ptr<Task> TaskMonitor::Add(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
                                       std::function<void(VkCommandBuffer)> task_callback,
                                       std::function<void()> callback) {
  gc();

  auto task = std::make_shared<Task>(fence, command, std::move(task_callback), std::move(callback));
  tasks_.push_back(task);
  return task;
}

void TaskMonitor::gc() {
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
