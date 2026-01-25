#ifndef VKGS_GPU_QUEUE_TASK_H
#define VKGS_GPU_QUEUE_TASK_H

#include <memory>
#include <vector>
#include <functional>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class Fence;
class Command;
class Object;

class QueueTaskImpl;
class VKGS_GPU_API QueueTask : public Handle<QueueTask, QueueTaskImpl> {
 public:
  static QueueTask Create(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                          std::function<void()> callback);

  bool IsDone();
  void Wait();
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_TASK_H
