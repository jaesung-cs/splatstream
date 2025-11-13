#ifndef VKGS_GPU_CMD_PIPELINE_H
#define VKGS_GPU_CMD_PIPELINE_H

#include <cstdint>
#include <map>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {
namespace cmd {

class VKGS_GPU_API Pipeline {
 public:
  Pipeline(VkPipelineBindPoint bind_point, VkPipelineLayout layout);
  ~Pipeline();

  Pipeline& Storage(int binding, VkBuffer buffer);

  Pipeline& Uniform(int binding, VkBuffer buffer);

  Pipeline& PushConstant(VkShaderStageFlags stage, uint32_t offset, uint32_t size, const void* values);

  Pipeline& Bind(VkPipeline pipeline);

  void Commit(VkCommandBuffer cb);

 private:
  VkPipelineBindPoint bind_point_;
  VkPipelineLayout layout_;

  struct DescriptorInfo {
    VkDescriptorType type;
    VkBuffer buffer;
  };
  std::map<int, DescriptorInfo> descriptors_;

  struct PushConstantData {
    VkShaderStageFlags stage;
    uint32_t offset;
    uint32_t size;
    const void* values;
  };
  std::vector<PushConstantData> push_constants_;

  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_CMD_PIPELINE_H
