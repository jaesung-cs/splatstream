#include "vkgs/gpu/cmd/pipeline.h"

#include <list>

#include <volk.h>

namespace vkgs {
namespace gpu {
namespace cmd {

Pipeline::Pipeline(VkPipelineBindPoint bind_point, VkPipelineLayout layout)
    : bind_point_(bind_point), layout_(layout) {}

Pipeline::~Pipeline() = default;

Pipeline& Pipeline::Storage(int binding, VkBuffer buffer) {
  buffer_descriptors_[binding] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer};
  return *this;
}

Pipeline& Pipeline::Uniform(int binding, VkBuffer buffer) {
  buffer_descriptors_[binding] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer};
  return *this;
}

Pipeline& Pipeline::Input(int binding, VkImageView image_view, VkImageLayout layout) {
  image_descriptors_[binding] = {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, image_view, layout};
  return *this;
}

Pipeline& Pipeline::PushConstant(VkShaderStageFlags stage, uint32_t offset, uint32_t size, const void* values) {
  push_constants_.push_back(PushConstantData{stage, offset, size, values});
  return *this;
}

Pipeline& Pipeline::Bind(VkPipeline pipeline) {
  pipeline_ = pipeline;
  return *this;
}

Pipeline& Pipeline::AttachmentLocations(const std::vector<uint32_t>& locations) {
  attachment_locations_ = locations;
  return *this;
}

Pipeline& Pipeline::InputAttachmentIndices(const std::vector<uint32_t>& indices) {
  input_attachment_indices_ = indices;
  return *this;
}

void Pipeline::Commit(VkCommandBuffer cb) {
  if (buffer_descriptors_.size() > 0 || image_descriptors_.size() > 0) {
    // List so that addresses of previous inserts never be invalidated.
    std::list<VkDescriptorBufferInfo> buffer_infos;
    std::list<VkDescriptorImageInfo> image_infos;
    std::vector<VkWriteDescriptorSet> writes;

    for (const auto& [binding, descriptor] : buffer_descriptors_) {
      auto& buffer_info = buffer_infos.emplace_back();
      buffer_info = {descriptor.buffer, 0, VK_WHOLE_SIZE};

      auto& write = writes.emplace_back();
      write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      write.dstBinding = binding;
      write.descriptorType = descriptor.type;
      write.descriptorCount = 1;
      write.pBufferInfo = &buffer_info;
    }

    for (const auto& [binding, descriptor] : image_descriptors_) {
      auto& image_info = image_infos.emplace_back();
      image_info = {VK_NULL_HANDLE, descriptor.image_view, descriptor.layout};

      auto& write = writes.emplace_back();
      write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      write.dstBinding = binding;
      write.descriptorType = descriptor.type;
      write.descriptorCount = 1;
      write.pImageInfo = &image_info;
    }

    vkCmdPushDescriptorSet(cb, bind_point_, layout_, 0, writes.size(), writes.data());
  }

  for (const auto& [stage, offset, size, values] : push_constants_) {
    vkCmdPushConstants(cb, layout_, stage, offset, size, values);
  }

  if (pipeline_) {
    vkCmdBindPipeline(cb, bind_point_, pipeline_);
  }

  if (!attachment_locations_.empty()) {
    VkRenderingAttachmentLocationInfo location_info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_LOCATION_INFO};
    location_info.colorAttachmentCount = attachment_locations_.size();
    location_info.pColorAttachmentLocations = attachment_locations_.data();
    vkCmdSetRenderingAttachmentLocations(cb, &location_info);
  }

  if (!input_attachment_indices_.empty()) {
    VkRenderingInputAttachmentIndexInfo input_attachment_index_info = {
        VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO};
    input_attachment_index_info.colorAttachmentCount = input_attachment_indices_.size();
    input_attachment_index_info.pColorAttachmentInputIndices = input_attachment_indices_.data();
    vkCmdSetRenderingInputAttachmentIndices(cb, &input_attachment_index_info);
  }

  buffer_descriptors_.clear();
  image_descriptors_.clear();
  push_constants_.clear();
  pipeline_ = VK_NULL_HANDLE;
  attachment_locations_.clear();
  input_attachment_indices_.clear();
}

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs
