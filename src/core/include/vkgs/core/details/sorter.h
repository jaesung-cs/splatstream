#ifndef VKGS_CORE_DETAILS_SORTER_H
#define VKGS_CORE_DETAILS_SORTER_H

#include "vk_radix_sort.h"

#include "vkgs/common/shared_accessor.h"

namespace vkgs {
namespace core {

class SorterImpl {
 public:
  explicit SorterImpl(VkDevice device, VkPhysicalDevice physical_device);
  ~SorterImpl();

  VrdxSorterStorageRequirements GetStorageRequirements(size_t max_size) const;
  void SortKeyValueIndirect(VkCommandBuffer cb, size_t max_size, VkBuffer size, VkBuffer key, VkBuffer value,
                            VkBuffer storage) const;

 private:
  VrdxSorter sorter_ = VK_NULL_HANDLE;
};

class Sorter : public SharedAccessor<Sorter, SorterImpl> {};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DETAILS_SORTER_H
