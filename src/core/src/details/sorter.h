#ifndef VKGS_CORE_DETAILS_SORTER_H
#define VKGS_CORE_DETAILS_SORTER_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

struct VrdxSorterStorageRequirements;

namespace vkgs {
namespace core {

class SorterImpl;
class Sorter : public Handle<Sorter, SorterImpl> {
 public:
  static Sorter Create(VkDevice device, VkPhysicalDevice physical_device);

  VrdxSorterStorageRequirements GetStorageRequirements(size_t max_size) const;
  void SortKeyValueIndirect(VkCommandBuffer cb, size_t max_size, VkBuffer size, VkBuffer key, VkBuffer value,
                            VkBuffer storage) const;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DETAILS_SORTER_H
