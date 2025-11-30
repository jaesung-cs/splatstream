#ifndef VKGS_GPU_TIMER_H
#define VKGS_GPU_TIMER_H

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

#include "export_api.h"
#include "object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API TimerImpl : public Object {
 public:
  TimerImpl(uint32_t size);
  ~TimerImpl() override;

  void Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage);
  std::vector<uint64_t> GetTimestamps() const;

 private:
  VkQueryPool query_pool_ = VK_NULL_HANDLE;
  uint32_t size_ = 0;
  uint32_t counter_ = 0;
};

class VKGS_GPU_API Timer : public SharedAccessor<Timer, TimerImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TIMER_H
