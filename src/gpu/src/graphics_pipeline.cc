#include "vkgs/gpu/graphics_pipeline.h"

#include <volk.h>

#include "vkgs/gpu/device.h"

#include "graphics_pipeline_pool.h"

namespace vkgs {
namespace gpu {

GraphicsPipelineImpl::GraphicsPipelineImpl(const GraphicsPipelineCreateInfo& create_info) {
  *this = device_->AllocateGraphicsPipeline(create_info);
}

GraphicsPipelineImpl::GraphicsPipelineImpl(std::shared_ptr<GraphicsPipelinePool> graphics_pipeline_pool,
                                           const GraphicsPipelineCreateInfo& create_info, VkPipeline pipeline)
    : graphics_pipeline_pool_(graphics_pipeline_pool), create_info_(create_info), pipeline_(pipeline) {}

GraphicsPipelineImpl::~GraphicsPipelineImpl() { graphics_pipeline_pool_->Free(create_info_, pipeline_); }

}  // namespace gpu
}  // namespace vkgs
