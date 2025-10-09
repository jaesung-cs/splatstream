#include "descriptor_set_layout.h"

namespace vkgs {
namespace core {

std::shared_ptr<DescriptorSetLayout> DescriptorSetLayout::Create(
    VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
  return std::make_shared<DescriptorSetLayout>(device, bindings);
}

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    : device_(device) {
  VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.bindingCount = bindings.size();
  layout_info.pBindings = bindings.data();
  vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &layout_);
}

DescriptorSetLayout::~DescriptorSetLayout() { vkDestroyDescriptorSetLayout(device_, layout_, nullptr); }

}  // namespace core
}  // namespace vkgs
