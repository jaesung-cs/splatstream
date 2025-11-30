#ifndef VKGS_GPU_TASK_MONITOR_H
#define VKGS_GPU_TASK_MONITOR_H

#include <memory>
#include <vector>
#include <functional>

#include "vkgs/common/shared_accessor.h"

namespace vkgs {
namespace gpu {

class QueueTask;
class Command;
class Fence;
class Object;

class TaskMonitorImpl {
 public:
  TaskMonitorImpl();
  ~TaskMonitorImpl();

  void FinishAllTasks();

  QueueTask Add(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                std::function<void()> callback);

 private:
  void gc();

  std::vector<QueueTask> tasks_;
  int rotation_index_ = 0;
};

class TaskMonitor : public SharedAccessor<TaskMonitor, TaskMonitorImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_MONITOR_H
