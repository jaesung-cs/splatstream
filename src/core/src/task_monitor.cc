#include "task_monitor.h"

#include "task.h"

namespace vkgs {
namespace core {

TaskMonitor::TaskMonitor() = default;

TaskMonitor::~TaskMonitor() = default;

void TaskMonitor::Add(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects) {
  gc();
  tasks_.push_back(std::make_shared<Task>(fence, std::move(objects)));
}

void TaskMonitor::gc() {
  constexpr int LOOP = 5;

  for (int i = 0; i < LOOP && i < tasks_.size(); ++i) {
    rotation_index_ = (rotation_index_ + 1) % tasks_.size();

    if (tasks_[i]->IsDone()) {
      std::swap(tasks_[i], tasks_.back());
      tasks_.pop_back();
    }
  }
}

}  // namespace core
}  // namespace vkgs
