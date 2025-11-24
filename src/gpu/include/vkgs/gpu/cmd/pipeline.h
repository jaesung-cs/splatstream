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

  Pipeline& Input(int binding, VkImageView image_view, VkImageLayout layout);

  Pipeline& PushConstant(VkShaderStageFlags stage, uint32_t offset, uint32_t size, const void* values);

  Pipeline& Bind(VkPipeline pipeline);

  Pipeline& AttachmentLocations(const std::vector<uint32_t>& locations);

  Pipeline& InputAttachmentIndices(const std::vector<uint32_t>& indices);

  void Commit(VkCommandBuffer cb);

 private:
  VkPipelineBindPoint bind_point_;
  VkPipelineLayout layout_;

  struct BufferDescriptorInfo {
    VkDescriptorType type;
    VkBuffer buffer;
  };
  std::map<int, BufferDescriptorInfo> buffer_descriptors_;

  struct ImageDescriptorInfo {
    VkDescriptorType type;
    VkImageView image_view;
    VkImageLayout layout;
  };
  std::map<int, ImageDescriptorInfo> image_descriptors_;

  struct PushConstantData {
    VkShaderStageFlags stage;
    uint32_t offset;
    uint32_t size;
    const void* values;
  };
  std::vector<PushConstantData> push_constants_;

  VkPipeline pipeline_ = VK_NULL_HANDLE;

  std::vector<uint32_t> attachment_locations_;
  std::vector<uint32_t> input_attachment_indices_;
};

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_CMD_PIPELINE_H
