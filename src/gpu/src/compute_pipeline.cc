#include "vkgs/gpu/compute_pipeline.h"

#include <volk.h>

namespace vkgs {
namespace gpu {

ComputePipelineImpl::ComputePipelineImpl(VkPipelineLayout pipeline_layout, const uint32_t* shader, size_t size) {
  VkShaderModuleCreateInfo shader_module_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  shader_module_info.codeSize = size * sizeof(uint32_t);
  shader_module_info.pCode = shader;
  VkShaderModule shader_module;
  vkCreateShaderModule(device_, &shader_module_info, NULL, &shader_module);

  VkComputePipelineCreateInfo pipeline_info = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  pipeline_info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipeline_info.stage.module = shader_module;
  pipeline_info.stage.pName = "main";
  pipeline_info.layout = pipeline_layout;

  vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline_);
  vkDestroyShaderModule(device_, shader_module, NULL);
}

ComputePipelineImpl::~ComputePipelineImpl() { vkDestroyPipeline(device_, pipeline_, NULL); }

}  // namespace gpu
}  // namespace vkgs
