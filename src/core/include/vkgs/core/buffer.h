#ifndef VKGS_CORE_BUFFER_H
#define VKGS_CORE_BUFFER_H

#include <string>
#include <memory>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class Module;
class Semaphore;
class Queue;

class VKGS_CORE_API Buffer : public std::enable_shared_from_this<Buffer> {
 public:
  Buffer(std::shared_ptr<Module> module, size_t size);
  ~Buffer();

  size_t size() const noexcept { return size_; }
  VkBuffer buffer() const noexcept { return buffer_; }
  VkBuffer stage_buffer() const noexcept { return stage_buffer_; }
  void* stage_buffer_map() const noexcept { return stage_buffer_map_; }
  auto semaphore() const noexcept { return semaphore_; }
  auto queue() const noexcept { return queue_; }

  void ToGpu(const void* ptr, size_t size);
  void ToCpu(void* ptr, size_t size);
  void Fill(uint32_t value);
  void Sort();

  void Wait();

  void WaitOn(std::shared_ptr<Semaphore> semaphore, std::shared_ptr<Queue> queue, VkPipelineStageFlags2 stage_mask,
              VkAccessFlags2 access_mask);

 private:
  std::shared_ptr<Module> module_;
  size_t size_ = 0;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;

  VkBuffer stage_buffer_ = VK_NULL_HANDLE;
  VmaAllocation stage_allocation_ = VK_NULL_HANDLE;
  void* stage_buffer_map_ = nullptr;

  // Semaphore to wait on.
  std::shared_ptr<Semaphore> semaphore_;

  // Queue by which the object is owned.
  std::shared_ptr<Queue> queue_;
  VkPipelineStageFlags2 stage_mask_;
  VkAccessFlags2 access_mask_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_BUFFER_H
