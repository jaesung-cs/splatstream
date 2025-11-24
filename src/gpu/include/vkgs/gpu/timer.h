#ifndef VKGS_GPU_TIMER_H
#define VKGS_GPU_TIMER_H

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "export_api.h"
#include "object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API Timer : public Object {
 public:
  static std::shared_ptr<Timer> Create(uint32_t size);

 public:
  Timer(uint32_t size);
  ~Timer();

  void Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage);
  std::vector<uint64_t> GetTimestamps() const;

 private:
  VkQueryPool query_pool_ = VK_NULL_HANDLE;
  uint32_t size_ = 0;
  uint32_t counter_ = 0;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TIMER_H
