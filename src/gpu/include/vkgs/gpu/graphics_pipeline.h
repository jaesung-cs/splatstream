#ifndef VKGS_GPU_GRAPHICS_PIPELINE_H
#define VKGS_GPU_GRAPHICS_PIPELINE_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

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
  std::vector<VkFormat> formats;
};

class VKGS_GPU_API GraphicsPipeline {
 public:
  static std::shared_ptr<GraphicsPipeline> Create(VkDevice device, const GraphicsPipelineCreateInfo& create_info);
  static void Terminate();

  GraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo& create_info);

  ~GraphicsPipeline();

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  VkDevice device_;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GRAPHICS_PIPELINE_H
