#ifndef VKGS_CORE_SORTER_H
#define VKGS_CORE_SORTER_H

#include "vk_radix_sort.h"

namespace vkgs {
namespace core {

class Sorter {
 public:
  explicit Sorter(VkDevice device, VkPhysicalDevice physical_device);
  ~Sorter();

  VrdxSorterStorageRequirements GetStorageRequirements(size_t max_size) const;
  void SortKeyValueIndirect(VkCommandBuffer cb, size_t max_size, VkBuffer size, VkBuffer key, VkBuffer value,
                            VkBuffer storage) const;

 private:
  VrdxSorter sorter_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SORTER_H
