#include "vkgs/gpu/cmd/barrier.h"

namespace vkgs {
namespace gpu {
namespace cmd {

Barrier::Barrier() = default;

Barrier::~Barrier() = default;

Barrier& Barrier::Release(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, uint32_t src_queue_family_index,
                          uint32_t dst_queue_family_index, VkBuffer buffer) {
  VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
  barrier.srcStageMask = src_stage;
  barrier.srcAccessMask = src_access;
  barrier.srcQueueFamilyIndex = src_queue_family_index;
  barrier.dstQueueFamilyIndex = dst_queue_family_index;
  barrier.buffer = buffer;
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;
  buffer_barriers_.push_back(barrier);
  return *this;
}

Barrier& Barrier::Release(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkImageLayout old_layout,
                          VkImageLayout new_layout, uint32_t src_queue_family_index, uint32_t dst_queue_family_index,
                          VkImage image) {
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.srcStageMask = src_stage;
  barrier.srcAccessMask = src_access;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = src_queue_family_index;
  barrier.dstQueueFamilyIndex = dst_queue_family_index;
  barrier.image = image;
  barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  image_barriers_.push_back(barrier);
  return *this;
}

Barrier& Barrier::Acquire(VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, uint32_t src_queue_family_index,
                          uint32_t dst_queue_family_index, VkBuffer buffer) {
  VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
  barrier.dstStageMask = dst_stage;
  barrier.dstAccessMask = dst_access;
  barrier.srcQueueFamilyIndex = src_queue_family_index;
  barrier.dstQueueFamilyIndex = dst_queue_family_index;
  barrier.buffer = buffer;
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;
  buffer_barriers_.push_back(barrier);
  return *this;
}

Barrier& Barrier::Acquire(VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, VkImageLayout old_layout,
                          VkImageLayout new_layout, uint32_t src_queue_family_index, uint32_t dst_queue_family_index,
                          VkImage image) {
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.dstStageMask = dst_stage;
  barrier.dstAccessMask = dst_access;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = src_queue_family_index;
  barrier.dstQueueFamilyIndex = dst_queue_family_index;
  barrier.image = image;
  barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  image_barriers_.push_back(barrier);
  return *this;
}

Barrier& Barrier::Memory(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkPipelineStageFlags2 dst_stage,
                         VkAccessFlags2 dst_access) {
  VkMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
  barrier.srcStageMask = src_stage;
  barrier.srcAccessMask = src_access;
  barrier.dstStageMask = dst_stage;
  barrier.dstAccessMask = dst_access;
  memory_barriers_.push_back(barrier);
  return *this;
}

Barrier& Barrier::Image(VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkPipelineStageFlags2 dst_stage,
                        VkAccessFlags2 dst_access, VkImageLayout old_layout, VkImageLayout new_layout, VkImage image) {
  VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  barrier.srcStageMask = src_stage;
  barrier.srcAccessMask = src_access;
  barrier.dstStageMask = dst_stage;
  barrier.dstAccessMask = dst_access;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.image = image;
  barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  image_barriers_.push_back(barrier);
  return *this;
}

void Barrier::Commit(VkCommandBuffer cb) {
  VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  dependency_info.memoryBarrierCount = memory_barriers_.size();
  dependency_info.pMemoryBarriers = memory_barriers_.data();
  dependency_info.bufferMemoryBarrierCount = buffer_barriers_.size();
  dependency_info.pBufferMemoryBarriers = buffer_barriers_.data();
  dependency_info.imageMemoryBarrierCount = image_barriers_.size();
  dependency_info.pImageMemoryBarriers = image_barriers_.data();
  vkCmdPipelineBarrier2(cb, &dependency_info);

  memory_barriers_.clear();
  buffer_barriers_.clear();
  image_barriers_.clear();
}

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs
