#ifndef VKGS_CORE_DETAILS_COMPUTE_STORAGE_H
#define VKGS_CORE_DETAILS_COMPUTE_STORAGE_H

#include <memory>
#include <cstdint>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

class ComputeStorageImpl {
 public:
  ComputeStorageImpl();
  ~ComputeStorageImpl();

  auto camera() const noexcept { return camera_; }
  auto camera_stage() const noexcept { return camera_stage_; }
  auto key() const noexcept { return key_; }
  auto index() const noexcept { return index_; }
  auto sort_storage() const noexcept { return sort_storage_; }
  auto inverse_index() const noexcept { return inverse_index_; }

  void Update(uint32_t point_count, VkBufferUsageFlags usage, VkDeviceSize size);

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

class ComputeStorage : public SharedAccessor<ComputeStorage, ComputeStorageImpl> {};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DETAILS_COMPUTE_STORAGE_H
