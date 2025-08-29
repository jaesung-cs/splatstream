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
class Semaphore;

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

  void WaitForRead();
  void WaitForWrite();

  // Internal synchronization.
  const auto& ReadWaitInfos() const noexcept { return read_wait_infos_; }
  const auto& WriteWaitInfos() const noexcept { return write_wait_infos_; }
  void ClearWaitInfo();
  void WaitOnRead(std::shared_ptr<Semaphore> semaphore, VkPipelineStageFlags2 stage_mask);
  void WaitOnWrite(std::shared_ptr<Semaphore> semaphore, VkPipelineStageFlags2 stage_mask, VkAccessFlags2 access_mask);

 private:
  std::shared_ptr<Module> module_;
  size_t size_ = 0;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;

  VkBuffer stage_buffer_ = VK_NULL_HANDLE;
  VmaAllocation stage_allocation_ = VK_NULL_HANDLE;
  void* stage_buffer_map_ = nullptr;

  // Semaphores to wait on.
  struct ReadWaitInfo {
    std::shared_ptr<Semaphore> semaphore;
    VkPipelineStageFlags2 stage_mask;
  };
  struct WriteWaitInfo {
    std::shared_ptr<Semaphore> semaphore;
    VkPipelineStageFlags2 stage_mask;
    VkAccessFlags2 access_mask;
  };

  std::vector<ReadWaitInfo> read_wait_infos_;
  std::vector<WriteWaitInfo> write_wait_infos_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_BUFFER_H
