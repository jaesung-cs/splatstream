#include "pipeline_layout.h"

namespace vkgs {
namespace core {

std::shared_ptr<PipelineLayout> PipelineLayout::Create(VkDevice device,
                                                       const std::vector<VkDescriptorSetLayout>& set_layouts,
                                                       const std::vector<VkPushConstantRange>& push_constants) {
  return std::make_shared<PipelineLayout>(device, set_layouts, push_constants);
}

PipelineLayout::PipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayout>& set_layouts,
                               const std::vector<VkPushConstantRange>& push_constants)
    : device_(device) {
  VkPipelineLayoutCreateInfo pipeline_layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipeline_layout_info.setLayoutCount = set_layouts.size();
  pipeline_layout_info.pSetLayouts = set_layouts.data();
  pipeline_layout_info.pushConstantRangeCount = push_constants.size();
  pipeline_layout_info.pPushConstantRanges = push_constants.data();
  vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &layout_);
}

PipelineLayout::~PipelineLayout() { vkDestroyPipelineLayout(device_, layout_, nullptr); }

}  // namespace core
}  // namespace vkgs
