#ifndef VKGS_GPU_QUEUE_TASK_H
#define VKGS_GPU_QUEUE_TASK_H

#include <memory>
#include <vector>
#include <functional>

#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/fence.h"
#include "vkgs/gpu/command.h"

namespace vkgs {
namespace gpu {

class Object;

class VKGS_GPU_API QueueTaskImpl {
 public:
  QueueTaskImpl(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                std::function<void()> callback);

  ~QueueTaskImpl();

  bool IsDone();
  void Wait();

 private:
  Fence fence_;
  Command command_;
  std::vector<std::shared_ptr<Object>> objects_;
  std::function<void()> callback_;
};

class VKGS_GPU_API QueueTask : public SharedAccessor<QueueTask, QueueTaskImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_TASK_H
