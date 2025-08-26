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

class VKGS_CORE_API Buffer : public std::enable_shared_from_this<Buffer> {
 public:
  Buffer(std::shared_ptr<Module> module, size_t size);
  ~Buffer();

  size_t size() const noexcept { return size_; }
  VkBuffer buffer() const noexcept { return buffer_; }
  VkBuffer stage_buffer() const noexcept { return stage_buffer_; }
  void* stage_buffer_map() const noexcept { return stage_buffer_map_; }

  void ToGpu(void* ptr, size_t size);

  void Wait();

  void WaitOn(std::shared_ptr<Semaphore> semaphore);

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
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_BUFFER_H
