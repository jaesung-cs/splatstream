#ifndef VKGS_CORE_SORTER_H
#define VKGS_CORE_SORTER_H

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vk_radix_sort.h"

namespace vkgs {
namespace core {

class Sorter {
 public:
  explicit Sorter(VkDevice device, VkPhysicalDevice physical_device, VmaAllocator allocator);
  ~Sorter();

  void Sort(VkCommandBuffer cb, VkBuffer buffer, size_t size);

 private:
  VkDevice device_;
  VkPhysicalDevice physical_device_;
  VmaAllocator allocator_;

  VrdxSorter sorter_ = VK_NULL_HANDLE;

  VkBuffer storage_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  VkDeviceSize storage_size_ = 0;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SORTER_H
