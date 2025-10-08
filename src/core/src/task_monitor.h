#ifndef VKGS_CORE_TASK_MONITOR_H
#define VKGS_CORE_TASK_MONITOR_H

#include <memory>
#include <vector>

namespace vkgs {
namespace core {

class Task;
class Command;
class Fence;

class TaskMonitor {
 public:
  TaskMonitor();
  ~TaskMonitor();

  void Add(std::shared_ptr<Command> command, std::shared_ptr<Fence> fence);

 private:
  void gc();

  std::vector<std::shared_ptr<Task>> tasks_;
  int rotation_index_ = 0;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TASK_MONITOR_H
