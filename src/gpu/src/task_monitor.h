#ifndef VKGS_GPU_TASK_MONITOR_H
#define VKGS_GPU_TASK_MONITOR_H

#include <memory>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

namespace vkgs {
namespace gpu {

class Task;
class Fence;
class Command;

class TaskMonitor {
 public:
  TaskMonitor();
  ~TaskMonitor();

  std::shared_ptr<Task> Add(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
                            std::function<void(VkCommandBuffer)> task_callback, std::function<void()> callback = {});

 private:
  void gc();

  std::vector<std::shared_ptr<Task>> tasks_;
  int rotation_index_ = 0;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_MONITOR_H
