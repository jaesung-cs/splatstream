#ifndef VKGS_CORE_DESCRIPTOR_SET_LAYOUT_H
#define VKGS_CORE_DESCRIPTOR_SET_LAYOUT_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace core {

class DescriptorSetLayout {
 public:
  static std::shared_ptr<DescriptorSetLayout> Create(VkDevice device,
                                                     const std::vector<VkDescriptorSetLayoutBinding>& bindings);

 public:
  DescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
  ~DescriptorSetLayout();

  operator VkDescriptorSetLayout() const noexcept { return layout_; }

 private:
  VkDevice device_;
  VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DESCRIPTOR_SET_LAYOUT_H
