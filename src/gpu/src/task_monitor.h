#ifndef VKGS_GPU_TASK_MONITOR_H
#define VKGS_GPU_TASK_MONITOR_H

#include <memory>
#include <vector>
#include <functional>

#include "vkgs/gpu/fence.h"

namespace vkgs {
namespace gpu {

class QueueTask;
class Command;

class TaskMonitor {
 public:
  TaskMonitor();
  ~TaskMonitor();

  void FinishAllTasks();

  QueueTask Add(Fence fence, std::shared_ptr<Command> command, std::vector<std::shared_ptr<Object>> objects,
                std::function<void()> callback);

 private:
  void gc();

  std::vector<QueueTask> tasks_;
  int rotation_index_ = 0;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_MONITOR_H
