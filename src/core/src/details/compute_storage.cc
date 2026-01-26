#include "details/compute_storage.h"

#include "vkgs/gpu/buffer.h"

#include "../struct.h"

namespace vkgs {
namespace core {

class ComputeStorageImpl {
 public:
  void __init__() {
    camera_ =
        gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Camera));
  }

  auto camera() const noexcept { return camera_; }
  auto camera_stage() const noexcept { return camera_stage_; }
  auto key() const noexcept { return key_; }
  auto index() const noexcept { return index_; }
  auto sort_storage() const noexcept { return sort_storage_; }
  auto inverse_index() const noexcept { return inverse_index_; }

  void Update(uint32_t point_count, VkBufferUsageFlags usage, VkDeviceSize size) {
    // Get new stage buffers
    camera_stage_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(Camera), true);

    if (point_count_ < point_count) {
      key_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(uint32_t));
      index_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(uint32_t));
      sort_storage_ = gpu::Buffer::Create(usage, size);
      inverse_index_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           point_count * sizeof(uint32_t));

      point_count_ = point_count;
    }
  }

 private:
  uint32_t point_count_ = 0;

  // Fixed
  gpu::Buffer camera_;        // (Camera)
  gpu::Buffer camera_stage_;  // (Camera)

  // Variable
  gpu::Buffer key_;            // (N)
  gpu::Buffer index_;          // (N)
  gpu::Buffer sort_storage_;   // (M)
  gpu::Buffer inverse_index_;  // (N)
};

ComputeStorage ComputeStorage::Create() { return Make<ComputeStorageImpl>(); }

gpu::Buffer ComputeStorage::camera() const { return impl_->camera(); }
gpu::Buffer ComputeStorage::camera_stage() const { return impl_->camera_stage(); }
gpu::Buffer ComputeStorage::key() const { return impl_->key(); }
gpu::Buffer ComputeStorage::index() const { return impl_->index(); }
gpu::Buffer ComputeStorage::sort_storage() const { return impl_->sort_storage(); }
gpu::Buffer ComputeStorage::inverse_index() const { return impl_->inverse_index(); }

void ComputeStorage::Update(uint32_t point_count, VkBufferUsageFlags usage, VkDeviceSize size) {
  impl_->Update(point_count, usage, size);
}

}  // namespace core
}  // namespace vkgs
