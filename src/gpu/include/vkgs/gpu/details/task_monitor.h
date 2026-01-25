#ifndef VKGS_GPU_DETAILS_TASK_MONITOR_H
#define VKGS_GPU_DETAILS_TASK_MONITOR_H

#include <memory>
#include <vector>
#include <functional>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class QueueTask;
class Command;
class Fence;
class Object;

class TaskMonitorImpl;
class TaskMonitor : public Handle<TaskMonitor, TaskMonitorImpl> {
 public:
  static TaskMonitor Create();

  void FinishAllTasks();
  QueueTask Add(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                std::function<void()> callback);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_TASK_MONITOR_H
