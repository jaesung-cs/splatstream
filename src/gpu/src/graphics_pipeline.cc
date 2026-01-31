#include "vkgs/gpu/graphics_pipeline.h"

#include "volk.h"

#include "vkgs/gpu/object.h"
#include "vkgs/gpu/gpu.h"

#include "details/graphics_pipeline_pool.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API GraphicsPipelineImpl : public Object {
 public:
  void __init__(GraphicsPipelinePool graphics_pipeline_pool, const GraphicsPipelineCreateInfo& create_info,
                VkPipeline pipeline) {
    graphics_pipeline_pool_ = graphics_pipeline_pool;
    create_info_ = create_info;
    pipeline_ = pipeline;
  }

  void __del__() { graphics_pipeline_pool_.Free(create_info_, pipeline_); }

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  GraphicsPipelinePool graphics_pipeline_pool_;
  GraphicsPipelineCreateInfo create_info_;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

GraphicsPipeline GraphicsPipeline::Create(const GraphicsPipelineCreateInfo& create_info) {
  // TODO: create from handle
  auto device = GetDevice();
  return device.AllocateGraphicsPipeline(create_info);
}

GraphicsPipeline GraphicsPipeline::Create(GraphicsPipelinePool graphics_pipeline_pool,
                                          const GraphicsPipelineCreateInfo& create_info, VkPipeline pipeline) {
  return Make<GraphicsPipelineImpl>(graphics_pipeline_pool, create_info, pipeline);
}

GraphicsPipeline::operator VkPipeline() const { return *impl_; }

void GraphicsPipeline::Keep() const { impl_->Keep(); }

}  // namespace gpu
}  // namespace vkgs
