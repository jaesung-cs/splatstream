#ifndef VKGS_CORE_BUFFER_H
#define VKGS_CORE_BUFFER_H

#include <memory>
#include <optional>
#include <vector>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class Module;
class Queue;

class VKGS_CORE_API Buffer : public std::enable_shared_from_this<Buffer> {
 public:
  Buffer(std::shared_ptr<Module> module, size_t size);
  ~Buffer();

  size_t size() const noexcept { return size_; }
  VkBuffer buffer() const noexcept { return buffer_; }
  VkBuffer stage_buffer() const noexcept { return stage_buffer_; }
  void* stage_buffer_map() const noexcept { return stage_buffer_map_; }

  void ToGpu(const void* ptr, size_t size);
  void ToCpu(void* ptr, size_t size);
  void Fill(uint32_t value);
  void Sort();

  // Internal synchronization.
  auto queue() const noexcept { return queue_; }
  auto timeline() const noexcept { return timeline_; }
  auto read_stage_mask() const noexcept { return read_stage_mask_; }
  auto write_stage_mask() const noexcept { return write_stage_mask_; }
  auto write_access_mask() const noexcept { return write_access_mask_; }

  void SetQueueTimeline(std::shared_ptr<Queue> queue, uint64_t timeline) {
    queue_ = queue;
    timeline_ = timeline;
  }
  void AddReadStageMask(VkPipelineStageFlags2 stage) { read_stage_mask_ |= stage; }
  void SetReadStageMask(VkPipelineStageFlags2 stage) { read_stage_mask_ = stage; }
  void SetWriteStageMask(VkPipelineStageFlags2 stage) { write_stage_mask_ = stage; }
  void SetWriteAccessMask(VkAccessFlags2 access) { write_access_mask_ = access; }

 private:
  std::shared_ptr<Module> module_;
  size_t size_ = 0;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;

  VkBuffer stage_buffer_ = VK_NULL_HANDLE;
  VmaAllocation stage_allocation_ = VK_NULL_HANDLE;
  void* stage_buffer_map_ = nullptr;

  std::shared_ptr<Queue> queue_;
  uint64_t timeline_ = 0;
  VkPipelineStageFlags2 read_stage_mask_ = 0;
  VkPipelineStageFlags2 write_stage_mask_ = 0;
  VkAccessFlags2 write_access_mask_ = 0;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_BUFFER_H
