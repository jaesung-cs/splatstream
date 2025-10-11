#include "sorter.h"

namespace vkgs {
namespace core {

Sorter::Sorter(VkDevice device, VkPhysicalDevice physical_device) {
  VrdxSorterCreateInfo sorter_info = {};
  sorter_info.physicalDevice = physical_device;
  sorter_info.device = device;
  vrdxCreateSorter(&sorter_info, &sorter_);
}

Sorter::~Sorter() { vrdxDestroySorter(sorter_); }

VrdxSorterStorageRequirements Sorter::GetStorageRequirements(size_t max_size) const {
  VrdxSorterStorageRequirements requirements;
  vrdxGetSorterKeyValueStorageRequirements(sorter_, max_size, &requirements);
  return requirements;
}

void Sorter::SortKeyValueIndirect(VkCommandBuffer cb, size_t max_size, VkBuffer size, VkBuffer key, VkBuffer value,
                                  VkBuffer storage) const {
  vrdxCmdSortKeyValueIndirect(cb, sorter_, max_size, size, 0, key, 0, value, 0, storage, 0, VK_NULL_HANDLE, 0);
}

}  // namespace core
}  // namespace vkgs
