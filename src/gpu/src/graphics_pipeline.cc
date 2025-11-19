#include "vkgs/gpu/graphics_pipeline.h"

#include <map>
#include <array>

#include <volk.h>

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/gpu.h"

#include "graphics_pipeline_pool.h"

namespace vkgs {
namespace gpu {

std::shared_ptr<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineCreateInfo& create_info) {
  auto device = GetDevice();
  return device->GetGraphicsPipelinePool()->Allocate(create_info);
}

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<GraphicsPipelinePool> graphics_pipeline_pool,
                                   const GraphicsPipelineCreateInfo& create_info, VkPipeline pipeline)
    : graphics_pipeline_pool_(graphics_pipeline_pool), create_info_(create_info), pipeline_(pipeline) {}

GraphicsPipeline::~GraphicsPipeline() { graphics_pipeline_pool_->Free(create_info_, pipeline_); }

}  // namespace gpu
}  // namespace vkgs
