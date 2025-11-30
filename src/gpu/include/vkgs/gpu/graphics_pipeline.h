#ifndef VKGS_GPU_GRAPHICS_PIPELINE_H
#define VKGS_GPU_GRAPHICS_PIPELINE_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class Device;

struct VKGS_GPU_API ShaderCode {
  ShaderCode() {}

  template <size_t N>
  ShaderCode(const uint32_t (&data)[N]) : data(data), size(N * sizeof(uint32_t)) {}

  const uint32_t* data = nullptr;
  size_t size = 0;
};

struct GraphicsPipelineCreateInfo {
  VkPipelineLayout pipeline_layout;
  ShaderCode vertex_shader;
  ShaderCode fragment_shader;
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  std::vector<VkFormat> formats;
  std::vector<uint32_t> locations;
  std::vector<uint32_t> input_indices;
  VkFormat depth_format;
  bool depth_test;
  bool depth_write;
};

class GraphicsPipelinePool;

class VKGS_GPU_API GraphicsPipelineImpl : public Object {
 public:
  GraphicsPipelineImpl(const GraphicsPipelineCreateInfo& create_info);

  GraphicsPipelineImpl(GraphicsPipelinePool graphics_pipeline_pool, const GraphicsPipelineCreateInfo& create_info,
                       VkPipeline pipeline);

  ~GraphicsPipelineImpl() override;

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  GraphicsPipelinePool graphics_pipeline_pool_;
  GraphicsPipelineCreateInfo create_info_;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

class VKGS_GPU_API GraphicsPipeline : public SharedAccessor<GraphicsPipeline, GraphicsPipelineImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GRAPHICS_PIPELINE_H
