#ifndef VKGS_GPU_GRAPHICS_PIPELINE_H
#define VKGS_GPU_GRAPHICS_PIPELINE_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "export_api.h"

#include "object.h"

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
  std::vector<VkFormat> formats;
  std::vector<uint32_t> locations;
  std::vector<uint32_t> input_indices;
};

class VKGS_GPU_API GraphicsPipeline : public Object {
 public:
  static std::shared_ptr<GraphicsPipeline> Create(std::shared_ptr<Device> device,
                                                  const GraphicsPipelineCreateInfo& create_info);
  static void Terminate();

  GraphicsPipeline(std::shared_ptr<Device> device, const GraphicsPipelineCreateInfo& create_info);

  ~GraphicsPipeline() override;

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GRAPHICS_PIPELINE_H
