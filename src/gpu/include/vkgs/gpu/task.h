#ifndef VKGS_GPU_TASK_H
#define VKGS_GPU_TASK_H

#include <memory>
#include <vector>
#include <functional>

#include "export_api.h"

#include <vulkan/vulkan.h>

namespace vkgs {
namespace gpu {

class Fence;
class Command;

class VKGS_GPU_API Task {
 public:
  Task(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
       std::function<void(VkCommandBuffer)> task_callback, std::function<void()> callback);

  ~Task();

  bool IsDone();
  void Wait();

 private:
  std::shared_ptr<Fence> fence_;
  std::shared_ptr<Command> command_;
  std::function<void(VkCommandBuffer)> task_callback_;
  std::function<void()> callback_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_H
