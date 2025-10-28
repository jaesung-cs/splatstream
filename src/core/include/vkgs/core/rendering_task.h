#ifndef VKGS_CORE_RENDERING_TASK_H
#define VKGS_CORE_RENDERING_TASK_H

#include <vector>
#include <memory>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Fence;
class Buffer;

}  // namespace gpu

namespace core {

class VKGS_CORE_API RenderingTask {
 public:
  RenderingTask(std::vector<std::shared_ptr<gpu::Fence>> fences, std::shared_ptr<gpu::Buffer> image_stage,
                uint64_t size, uint8_t* dst);
  ~RenderingTask();

  void Wait();

 private:
  std::vector<std::shared_ptr<gpu::Fence>> fences_;
  std::shared_ptr<gpu::Buffer> image_stage_;
  uint64_t size_;
  uint8_t* dst_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERING_TASK_H
