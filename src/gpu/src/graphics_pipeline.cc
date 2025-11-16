#include "vkgs/gpu/graphics_pipeline.h"

#include <array>

#include <volk.h>

namespace vkgs {
namespace gpu {

GraphicsPipeline::GraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo& create_info) : device_(device) {
  // TODO: pipeline cache.
  VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

  VkShaderModuleCreateInfo vertex_shader_module_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  vertex_shader_module_info.codeSize = create_info.vertex_shader.size;
  vertex_shader_module_info.pCode = create_info.vertex_shader.data;
  VkShaderModule vertex_shader_module;
  vkCreateShaderModule(device_, &vertex_shader_module_info, NULL, &vertex_shader_module);

  VkShaderModuleCreateInfo fragment_shader_module_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  fragment_shader_module_info.codeSize = create_info.fragment_shader.size;
  fragment_shader_module_info.pCode = create_info.fragment_shader.data;
  VkShaderModule fragment_shader_module;
  vkCreateShaderModule(device_, &fragment_shader_module_info, NULL, &fragment_shader_module);

  std::array<VkPipelineShaderStageCreateInfo, 2> stages;
  stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vertex_shader_module;
  stages[0].pName = "main";
  stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fragment_shader_module;
  stages[1].pName = "main";

  VkPipelineRenderingCreateInfo rendering_info = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  rendering_info.colorAttachmentCount = create_info.formats.size();
  rendering_info.pColorAttachmentFormats = create_info.formats.data();

  VkPipelineVertexInputStateCreateInfo vertex_input_state = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo viewport_state = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterization_state = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_state.lineWidth = 1.f;

  VkPipelineMultisampleStateCreateInfo multisample_state = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  std::array<VkPipelineColorBlendAttachmentState, 3> color_attachments;
  color_attachments[0] = {};
  color_attachments[0].blendEnable = VK_TRUE;
  color_attachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  color_attachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
  color_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_attachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
  color_attachments[0].colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_attachments[1] = color_attachments[0];
  color_attachments[2] = color_attachments[0];

  VkPipelineColorBlendStateCreateInfo color_blend_state = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  color_blend_state.attachmentCount = color_attachments.size();
  color_blend_state.pAttachments = color_attachments.data();

  std::vector<VkDynamicState> dynamic_states = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamic_state.dynamicStateCount = dynamic_states.size();
  dynamic_state.pDynamicStates = dynamic_states.data();

  VkGraphicsPipelineCreateInfo pipeline_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipeline_info.pNext = &rendering_info;
  pipeline_info.stageCount = stages.size();
  pipeline_info.pStages = stages.data();
  pipeline_info.pVertexInputState = &vertex_input_state;
  pipeline_info.pInputAssemblyState = &input_assembly_state;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterization_state;
  pipeline_info.pMultisampleState = &multisample_state;
  pipeline_info.pColorBlendState = &color_blend_state;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = create_info.pipeline_layout;
  vkCreateGraphicsPipelines(device_, pipeline_cache, 1, &pipeline_info, NULL, &pipeline_);

  vkDestroyShaderModule(device_, vertex_shader_module, NULL);
  vkDestroyShaderModule(device_, fragment_shader_module, NULL);
}

GraphicsPipeline::~GraphicsPipeline() { vkDestroyPipeline(device_, pipeline_, NULL); }

}  // namespace gpu
}  // namespace vkgs
