#ifndef VKGS_GPU_PIPELINE_LAYOUT_H
#define VKGS_GPU_PIPELINE_LAYOUT_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API PipelineLayoutImpl : public Object {
 public:
  PipelineLayoutImpl(const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                     const std::vector<VkPushConstantRange>& push_constants);
  ~PipelineLayoutImpl() override;

  operator VkPipelineLayout() const noexcept { return pipeline_layout_; }

 private:
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

class VKGS_GPU_API PipelineLayout : public SharedAccessor<PipelineLayout, PipelineLayoutImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_PIPELINE_LAYOUT_H
