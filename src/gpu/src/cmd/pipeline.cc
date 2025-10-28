#include "vkgs/gpu/cmd/pipeline.h"

#include <list>

namespace vkgs {
namespace gpu {
namespace cmd {

Pipeline::Pipeline(VkPipelineBindPoint bind_point, VkPipelineLayout layout)
    : bind_point_(bind_point), layout_(layout) {}

Pipeline::~Pipeline() = default;

Pipeline& Pipeline::Storage(int binding, VkBuffer buffer) {
  descriptors_[binding] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer};
  return *this;
}

Pipeline& Pipeline::Uniform(int binding, VkBuffer buffer) {
  descriptors_[binding] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer};
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

void Pipeline::Commit(VkCommandBuffer cb) {
  if (descriptors_.size() > 0) {
    // List so that addresses of previous inserts never be invalidated.
    std::list<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
    for (const auto& [binding, descriptor] : descriptors_) {
      auto& buffer_info = buffer_infos.emplace_back();
      buffer_info = {descriptor.buffer, 0, VK_WHOLE_SIZE};

      auto& write = writes.emplace_back();
      write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
      write.dstBinding = binding;
      write.descriptorType = descriptor.type;
      write.descriptorCount = 1;
      write.pBufferInfo = &buffer_info;
    }
    vkCmdPushDescriptorSet(cb, bind_point_, layout_, 0, writes.size(), writes.data());
  }

  for (const auto& [stage, offset, size, values] : push_constants_) {
    vkCmdPushConstants(cb, layout_, stage, offset, size, values);
  }

  if (pipeline_) {
    vkCmdBindPipeline(cb, bind_point_, pipeline_);
  }

  descriptors_.clear();
  push_constants_.clear();
  pipeline_ = VK_NULL_HANDLE;
}

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs
