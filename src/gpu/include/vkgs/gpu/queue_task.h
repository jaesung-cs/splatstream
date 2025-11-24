#ifndef VKGS_GPU_QUEUE_TASK_H
#define VKGS_GPU_QUEUE_TASK_H

#include <memory>
#include <vector>
#include <functional>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Object;
class Fence;
class Command;

class VKGS_GPU_API QueueTask {
 public:
  QueueTask(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
            std::vector<std::shared_ptr<Object>> objects, std::function<void()> callback);

  ~QueueTask();

  bool IsDone();
  void Wait();

 private:
  std::shared_ptr<Fence> fence_;
  std::shared_ptr<Command> command_;
  std::vector<std::shared_ptr<Object>> objects_;
  std::function<void()> callback_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_TASK_H
