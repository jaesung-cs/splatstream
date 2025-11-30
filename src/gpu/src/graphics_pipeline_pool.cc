#include "vkgs/gpu/graphics_pipeline_pool.h"

#include <array>

#include <volk.h>

#include "vkgs/gpu/graphics_pipeline.h"

namespace vkgs {
namespace gpu {
namespace {

bool operator!=(const ShaderCode& lhs, const ShaderCode& rhs) { return lhs.size != rhs.size || lhs.data != rhs.data; }
bool operator<(const ShaderCode& lhs, const ShaderCode& rhs) {
  return lhs.size != rhs.size ? lhs.size < rhs.size : lhs.data < rhs.data;
}

bool operator!=(const VkVertexInputBindingDescription& lhs, const VkVertexInputBindingDescription& rhs) {
  return lhs.binding != rhs.binding || lhs.stride != rhs.stride || lhs.inputRate != rhs.inputRate;
}
bool operator<(const VkVertexInputBindingDescription& lhs, const VkVertexInputBindingDescription& rhs) {
  return lhs.binding != rhs.binding ? lhs.binding < rhs.binding
         : lhs.stride != rhs.stride ? lhs.stride < rhs.stride
                                    : lhs.inputRate < rhs.inputRate;
}

bool operator!=(const VkVertexInputAttributeDescription& lhs, const VkVertexInputAttributeDescription& rhs) {
  return lhs.binding != rhs.binding || lhs.location != rhs.location || lhs.format != rhs.format ||
         lhs.offset != rhs.offset;
}
bool operator<(const VkVertexInputAttributeDescription& lhs, const VkVertexInputAttributeDescription& rhs) {
  return lhs.binding != rhs.binding     ? lhs.binding < rhs.binding
         : lhs.location != rhs.location ? lhs.location < rhs.location
         : lhs.format != rhs.format     ? lhs.format < rhs.format
                                        : lhs.offset < rhs.offset;
}

}  // namespace

bool GraphicsPipelineCreateInfoLess::operator()(const GraphicsPipelineCreateInfo& lhs,
                                                const GraphicsPipelineCreateInfo& rhs) const {
  if (lhs.pipeline_layout != rhs.pipeline_layout) return lhs.pipeline_layout < rhs.pipeline_layout;
  if (lhs.vertex_shader != rhs.vertex_shader) return lhs.vertex_shader < rhs.vertex_shader;
  if (lhs.fragment_shader != rhs.fragment_shader) return lhs.fragment_shader < rhs.fragment_shader;
  if (lhs.bindings.size() != rhs.bindings.size()) return lhs.bindings.size() < rhs.bindings.size();
  for (size_t i = 0; i < lhs.bindings.size(); ++i) {
    if (lhs.bindings[i] != rhs.bindings[i]) return lhs.bindings[i] < rhs.bindings[i];
  }
  if (lhs.attributes.size() != rhs.attributes.size()) return lhs.attributes.size() < rhs.attributes.size();
  for (size_t i = 0; i < lhs.attributes.size(); ++i) {
    if (lhs.attributes[i] != rhs.attributes[i]) return lhs.attributes[i] < rhs.attributes[i];
  }
  if (lhs.topology != rhs.topology) return lhs.topology < rhs.topology;
  if (lhs.formats.size() != rhs.formats.size()) return lhs.formats.size() < rhs.formats.size();
  for (size_t i = 0; i < lhs.formats.size(); ++i) {
    if (lhs.formats[i] != rhs.formats[i]) return lhs.formats[i] < rhs.formats[i];
  }
  if (lhs.locations.size() != rhs.locations.size()) return lhs.locations.size() < rhs.locations.size();
  for (size_t i = 0; i < lhs.locations.size(); ++i) {
    if (lhs.locations[i] != rhs.locations[i]) return lhs.locations[i] < rhs.locations[i];
  }
  if (lhs.depth_format != rhs.depth_format) return lhs.depth_format < rhs.depth_format;
  if (lhs.depth_test != rhs.depth_test) return lhs.depth_test < rhs.depth_test;
  if (lhs.depth_write != rhs.depth_write) return lhs.depth_write < rhs.depth_write;
  return false;
}

GraphicsPipelinePoolImpl::GraphicsPipelinePoolImpl(VkDevice device) : device_(device) {}

GraphicsPipelinePoolImpl::~GraphicsPipelinePoolImpl() {
  for (auto& [_, pipeline] : pipelines_) {
    vkDestroyPipeline(device_, pipeline, NULL);
  }
}

GraphicsPipeline GraphicsPipelinePoolImpl::Allocate(const GraphicsPipelineCreateInfo& create_info) {
  auto it = pipelines_.find(create_info);
  if (it != pipelines_.end()) {
    return GraphicsPipeline::Create(GraphicsPipelinePool::FromPtr(shared_from_this()), create_info, it->second);
  }

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

  void* next = nullptr;
  VkRenderingInputAttachmentIndexInfo input_attachment_index_info = {
      VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO};
  input_attachment_index_info.colorAttachmentCount = create_info.input_indices.size();
  input_attachment_index_info.pColorAttachmentInputIndices = create_info.input_indices.data();
  if (!create_info.input_indices.empty()) next = &input_attachment_index_info;

  VkRenderingAttachmentLocationInfo location_info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_LOCATION_INFO};
  location_info.pNext = next;
  location_info.colorAttachmentCount = create_info.locations.size();
  location_info.pColorAttachmentLocations = create_info.locations.data();
  if (!create_info.locations.empty()) next = &location_info;

  VkPipelineRenderingCreateInfo rendering_info = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  rendering_info.pNext = next;
  rendering_info.colorAttachmentCount = create_info.formats.size();
  rendering_info.pColorAttachmentFormats = create_info.formats.data();
  rendering_info.depthAttachmentFormat = create_info.depth_format;

  VkPipelineVertexInputStateCreateInfo vertex_input_state = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertex_input_state.vertexBindingDescriptionCount = create_info.bindings.size();
  vertex_input_state.pVertexBindingDescriptions = create_info.bindings.data();
  vertex_input_state.vertexAttributeDescriptionCount = create_info.attributes.size();
  vertex_input_state.pVertexAttributeDescriptions = create_info.attributes.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  input_assembly_state.topology = create_info.topology;

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

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depth_stencil_state.depthTestEnable = create_info.depth_test;
  depth_stencil_state.depthWriteEnable = create_info.depth_write;
  depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  // TODO: blend equation
  std::vector<VkPipelineColorBlendAttachmentState> color_attachments(create_info.formats.size());
  for (auto& color_attachment : color_attachments) {
    color_attachment = {};
    color_attachment.blendEnable = VK_TRUE;
    color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  }

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
  pipeline_info.pDepthStencilState = &depth_stencil_state;
  pipeline_info.pMultisampleState = &multisample_state;
  pipeline_info.pColorBlendState = &color_blend_state;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = create_info.pipeline_layout;
  VkPipeline pipeline;
  vkCreateGraphicsPipelines(device_, pipeline_cache, 1, &pipeline_info, NULL, &pipeline);

  vkDestroyShaderModule(device_, vertex_shader_module, NULL);
  vkDestroyShaderModule(device_, fragment_shader_module, NULL);

  pipelines_[create_info] = pipeline;
  return GraphicsPipeline::Create(GraphicsPipelinePool::FromPtr(shared_from_this()), create_info, pipeline);
}

void GraphicsPipelinePoolImpl::Free(const GraphicsPipelineCreateInfo& create_info, VkPipeline pipeline) {
  pipelines_[create_info] = pipeline;
}

}  // namespace gpu
}  // namespace vkgs
