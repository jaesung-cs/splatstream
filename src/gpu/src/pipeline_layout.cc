#include "vkgs/gpu/pipeline_layout.h"

#include "volk.h"

#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class PipelineLayoutImpl : public Object {
 public:
  void __init__(const PipelineLayoutCreateInfo& create_info) {
    VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    layout_info.bindingCount = create_info.bindings.size();
    layout_info.pBindings = create_info.bindings.data();
    vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_);

    VkPipelineLayoutCreateInfo pipeline_layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
    pipeline_layout_info.pushConstantRangeCount = create_info.push_constants.size();
    pipeline_layout_info.pPushConstantRanges = create_info.push_constants.data();
    vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_);
  }

  void __del__() {
    vkDestroyPipelineLayout(device_, pipeline_layout_, NULL);
    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, NULL);
  }

  operator VkPipelineLayout() const noexcept { return pipeline_layout_; }

 private:
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

PipelineLayout PipelineLayout::Create(const PipelineLayoutCreateInfo& create_info) {
  return Make<PipelineLayoutImpl>(create_info);
}

PipelineLayout::operator VkPipelineLayout() const { return *impl_; }

}  // namespace gpu
}  // namespace vkgs
