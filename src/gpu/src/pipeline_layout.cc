#include "vkgs/gpu/pipeline_layout.h"

namespace vkgs {
namespace gpu {

std::shared_ptr<PipelineLayout> PipelineLayout::Create(VkDevice device,
                                                       const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                                                       const std::vector<VkPushConstantRange>& push_constants) {
  return std::make_shared<PipelineLayout>(device, bindings, push_constants);
}

PipelineLayout::PipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                               const std::vector<VkPushConstantRange>& push_constants)
    : device_(device) {
  VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
  layout_info.bindingCount = bindings.size();
  layout_info.pBindings = bindings.data();
  vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_);

  VkPipelineLayoutCreateInfo pipeline_layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
  pipeline_layout_info.pushConstantRangeCount = push_constants.size();
  pipeline_layout_info.pPushConstantRanges = push_constants.data();
  vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_);
}

PipelineLayout::~PipelineLayout() {
  vkDestroyPipelineLayout(device_, pipeline_layout_, NULL);
  vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, NULL);
}

}  // namespace gpu
}  // namespace vkgs
