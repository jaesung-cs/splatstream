#ifndef VKGS_GPU_DETAILS_GRAPHICS_PIPELINE_POOL_H
#define VKGS_GPU_DETAILS_GRAPHICS_PIPELINE_POOL_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;

class GraphicsPipelinePoolImpl;
class GraphicsPipelinePool : public Handle<GraphicsPipelinePool, GraphicsPipelinePoolImpl> {
 public:
  static GraphicsPipelinePool Create(VkDevice device);

  GraphicsPipeline Allocate(const GraphicsPipelineCreateInfo& create_info);
  void Free(const GraphicsPipelineCreateInfo& create_info, VkPipeline pipeline);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_GRAPHICS_PIPELINE_POOL_H
