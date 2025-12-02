#ifndef VKGS_GPU_DETAILS_GRAPHICS_PIPELINE_POOL_H
#define VKGS_GPU_DETAILS_GRAPHICS_PIPELINE_POOL_H

#include <memory>
#include <map>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

namespace vkgs {
namespace gpu {

struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;

struct GraphicsPipelineCreateInfoLess {
  bool operator()(const GraphicsPipelineCreateInfo& lhs, const GraphicsPipelineCreateInfo& rhs) const;
};

class GraphicsPipelinePoolImpl : public std::enable_shared_from_this<GraphicsPipelinePoolImpl> {
 public:
  GraphicsPipelinePoolImpl(VkDevice device);
  ~GraphicsPipelinePoolImpl();

  GraphicsPipeline Allocate(const GraphicsPipelineCreateInfo& create_info);
  void Free(const GraphicsPipelineCreateInfo& create_info, VkPipeline pipeline);

 private:
  VkDevice device_;
  std::map<GraphicsPipelineCreateInfo, VkPipeline, GraphicsPipelineCreateInfoLess> pipelines_;
};

class GraphicsPipelinePool : public SharedAccessor<GraphicsPipelinePool, GraphicsPipelinePoolImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_GRAPHICS_PIPELINE_POOL_H
