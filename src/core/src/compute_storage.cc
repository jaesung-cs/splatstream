#include "compute_storage.h"

#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

ComputeStorage::ComputeStorage(std::shared_ptr<gpu::Device> device) : device_(device) {
  visible_point_count_ = gpu::Buffer::Create(
      device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      sizeof(uint32_t));
  draw_indirect_ =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                          sizeof(VkDrawIndexedIndirectCommand));
}

ComputeStorage::~ComputeStorage() {}

void ComputeStorage::Update(uint32_t point_count, const VrdxSorterStorageRequirements& storage_requirements) {
  if (point_count_ < point_count) {
    key_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(uint32_t));
    index_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(uint32_t));
    sort_storage_ = gpu::Buffer::Create(device_, storage_requirements.usage, storage_requirements.size);
    inverse_index_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         point_count * sizeof(uint32_t));
    instances_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 12 * sizeof(float));

    point_count_ = point_count;
  }
}

}  // namespace core
}  // namespace vkgs