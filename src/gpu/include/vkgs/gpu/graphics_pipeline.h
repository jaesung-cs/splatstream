#ifndef VKGS_GPU_GRAPHICS_PIPELINE_H
#define VKGS_GPU_GRAPHICS_PIPELINE_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API GraphicsPipeline {
 public:
  template <size_t N, size_t M>
  static std::shared_ptr<GraphicsPipeline> Create(VkDevice device, VkPipelineLayout pipeline_layout,
                                                  const uint32_t (&vertex_shader)[N],
                                                  const uint32_t (&fragment_shader)[M], VkFormat format) {
    return std::make_shared<GraphicsPipeline>(device, pipeline_layout, vertex_shader, N, fragment_shader, M, format);
  }

  GraphicsPipeline(VkDevice device, VkPipelineLayout pipeline_layout, const uint32_t* vertex_shader,
                   size_t vertex_shader_size, const uint32_t* fragment_shader, size_t fragment_shader_size,
                   VkFormat format);

  ~GraphicsPipeline();

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  VkDevice device_;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GRAPHICS_PIPELINE_H
