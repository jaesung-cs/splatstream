#ifndef VKGS_GPU_CMD_BARRIER_H
#define VKGS_GPU_CMD_BARRIER_H

#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {
namespace cmd {

class VKGS_GPU_API Barrier {
 public:
  Barrier();
  ~Barrier();

  Barrier& Release(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, uint32_t src_queue_family_index,
                   uint32_t dst_queue_family_index, VkBuffer buffer);

  Barrier& Release(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkImageLayout old_layout,
                   VkImageLayout new_layout, uint32_t src_queue_family_index, uint32_t dst_queue_family_index,
                   VkImage image);

  Barrier& Acquire(VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, uint32_t src_queue_family_index,
                   uint32_t dst_queue_family_index, VkBuffer buffer);

  Barrier& Acquire(VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, VkImageLayout old_layout,
                   VkImageLayout new_layout, uint32_t src_queue_family_index, uint32_t dst_queue_family_index,
                   VkImage image);

  Barrier& Memory(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkPipelineStageFlags2 dst_stage,
                  VkAccessFlags2 dst_access);

  Barrier& Image(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkPipelineStageFlags2 dst_stage,
                 VkAccessFlags2 dst_access, VkImageLayout old_layout, VkImageLayout new_layout, VkImage image);

  void Commit(VkCommandBuffer cb);

 private:
  std::vector<VkMemoryBarrier2> memory_barriers_;
  std::vector<VkBufferMemoryBarrier2> buffer_barriers_;
  std::vector<VkImageMemoryBarrier2> image_barriers_;
};

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_CMD_BARRIER_H
