#ifndef VKGS_CORE_PIPELINE_LAYOUT_H
#define VKGS_CORE_PIPELINE_LAYOUT_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace core {

class PipelineLayout {
 public:
  static std::shared_ptr<PipelineLayout> Create(VkDevice device,
                                                const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                                                const std::vector<VkPushConstantRange>& push_constants);

 public:
  PipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                 const std::vector<VkPushConstantRange>& push_constants);
  ~PipelineLayout();

  operator VkPipelineLayout() const noexcept { return pipeline_layout_; }

 private:
  VkDevice device_;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_PIPELINE_LAYOUT_H
