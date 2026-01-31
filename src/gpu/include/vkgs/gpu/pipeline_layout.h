#ifndef VKGS_GPU_PIPELINE_LAYOUT_H
#define VKGS_GPU_PIPELINE_LAYOUT_H

#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

struct PipelineLayoutCreateInfo {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  std::vector<VkPushConstantRange> push_constants;
};

class PipelineLayoutImpl;
class VKGS_GPU_API PipelineLayout : public Handle<PipelineLayout, PipelineLayoutImpl> {
 public:
  static PipelineLayout Create(const PipelineLayoutCreateInfo& create_info);

  operator VkPipelineLayout() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_PIPELINE_LAYOUT_H
