#ifndef VKGS_GPU_TASK_MONITOR_H
#define VKGS_GPU_TASK_MONITOR_H

#include <memory>
#include <vector>
#include <functional>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Task;
class Fence;
class Object;

class VKGS_GPU_API TaskMonitor {
 public:
  TaskMonitor();
  ~TaskMonitor();

  std::shared_ptr<Task> Add(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects,
                            std::function<void()> callback = {});

 private:
  void gc();

  std::vector<std::shared_ptr<Task>> tasks_;
  int rotation_index_ = 0;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_MONITOR_H
