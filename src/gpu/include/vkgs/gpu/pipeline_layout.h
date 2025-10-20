#ifndef VKGS_GPU_PIPELINE_LAYOUT_H
#define VKGS_GPU_PIPELINE_LAYOUT_H

#include <memory>
#include <vector>

#include "volk.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API PipelineLayout {
 public:
  static std::shared_ptr<PipelineLayout> Create(VkDevice device,
                                                const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                                                const std::vector<VkPushConstantRange>& push_constants = {});

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

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_PIPELINE_LAYOUT_H
