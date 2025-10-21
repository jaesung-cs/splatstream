#include "compute_storage.h"

#include "vkgs/gpu/buffer.h"

#include "struct.h"

namespace vkgs {
namespace core {

ComputeStorage::ComputeStorage(std::shared_ptr<gpu::Device> device) : device_(device) {
  visible_point_count_ = gpu::Buffer::Create(
      device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      sizeof(uint32_t));
  camera_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                sizeof(Camera));
  draw_indirect_ =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                          sizeof(VkDrawIndexedIndirectCommand));
}

ComputeStorage::~ComputeStorage() {}

void ComputeStorage::Update(uint32_t point_count, const VrdxSorterStorageRequirements& storage_requirements) {
  // Get new stage buffers
  camera_stage_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(Camera), true);
  // TODO: fill the index buffer in projection pipeline.
  index_stage_ =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 6 * point_count * sizeof(uint32_t), true);

  if (point_count_ < point_count) {
    key_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(uint32_t));
    index_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(uint32_t));
    sort_storage_ = gpu::Buffer::Create(device_, storage_requirements.usage, storage_requirements.size);
    inverse_index_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         point_count * sizeof(uint32_t));
    instances_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 12 * sizeof(float));
    index_buffer_ = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        6 * point_count * sizeof(uint32_t));

    point_count_ = point_count;
  }
}

}  // namespace core
}  // namespace vkgs