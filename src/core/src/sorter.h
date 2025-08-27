#ifndef VKGS_CORE_SORTER_H
#define VKGS_CORE_SORTER_H

#include <memory>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vk_radix_sort.h"

namespace vkgs {
namespace core {

class Module;
class Buffer;

class Sorter {
 public:
  explicit Sorter(Module* module);
  ~Sorter();

  void Sort(VkCommandBuffer cb, std::shared_ptr<Buffer> buffer);

 private:
  Module* module_;
  VrdxSorter sorter_ = VK_NULL_HANDLE;

  VkBuffer storage_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  VkDeviceSize storage_size_ = 0;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SORTER_H
