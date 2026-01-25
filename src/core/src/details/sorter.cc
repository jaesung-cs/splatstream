#include "vkgs/core/details/sorter.h"

namespace vkgs {
namespace core {

class SorterImpl {
 public:
  void __init__(VkDevice device, VkPhysicalDevice physical_device) {
    VrdxSorterCreateInfo sorter_info = {};
    sorter_info.physicalDevice = physical_device;
    sorter_info.device = device;
    vrdxCreateSorter(&sorter_info, &sorter_);
  }

  void __del__() { vrdxDestroySorter(sorter_); }

  VrdxSorterStorageRequirements GetStorageRequirements(size_t max_size) const {
    VrdxSorterStorageRequirements requirements;
    vrdxGetSorterKeyValueStorageRequirements(sorter_, max_size, &requirements);
    return requirements;
  }

  void SortKeyValueIndirect(VkCommandBuffer cb, size_t max_size, VkBuffer size, VkBuffer key, VkBuffer value,
                            VkBuffer storage) const {
    vrdxCmdSortKeyValueIndirect(cb, sorter_, max_size, size, 0, key, 0, value, 0, storage, 0, VK_NULL_HANDLE, 0);
  }

 private:
  VrdxSorter sorter_ = VK_NULL_HANDLE;
};

Sorter Sorter::Create(VkDevice device, VkPhysicalDevice physical_device) {
  return Make<SorterImpl>(device, physical_device);
}

VrdxSorterStorageRequirements Sorter::GetStorageRequirements(size_t max_size) const {
  return impl_->GetStorageRequirements(max_size);
}

void Sorter::SortKeyValueIndirect(VkCommandBuffer cb, size_t max_size, VkBuffer size, VkBuffer key, VkBuffer value,
                                  VkBuffer storage) const {
  impl_->SortKeyValueIndirect(cb, max_size, size, key, value, storage);
}

}  // namespace core
}  // namespace vkgs
