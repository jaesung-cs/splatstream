#ifndef VKGS_GPU_TASK_MONITOR_H
#define VKGS_GPU_TASK_MONITOR_H

#include <memory>
#include <vector>

namespace vkgs {
namespace gpu {

class Task;
class Fence;
class Object;

class TaskMonitor {
 public:
  TaskMonitor();
  ~TaskMonitor();

  void Add(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects);

 private:
  void gc();

  std::vector<std::shared_ptr<Task>> tasks_;
  int rotation_index_ = 0;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_MONITOR_H
