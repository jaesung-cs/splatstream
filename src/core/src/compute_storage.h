#ifndef VKGS_CORE_COMPUTE_STORAGE_H
#define VKGS_CORE_COMPUTE_STORAGE_H

#include <memory>
#include <cstdint>

#include "vk_radix_sort.h"

namespace vkgs {
namespace gpu {

class Device;
class Buffer;

}  // namespace gpu

namespace core {

class ComputeStorage {
 public:
  ComputeStorage(std::shared_ptr<gpu::Device> device);
  ~ComputeStorage();

  auto visible_point_count() const noexcept { return visible_point_count_; }
  auto camera() const noexcept { return camera_; }
  auto draw_indirect() const noexcept { return draw_indirect_; }
  auto camera_stage() const noexcept { return camera_stage_; }
  auto key() const noexcept { return key_; }
  auto index() const noexcept { return index_; }
  auto sort_storage() const noexcept { return sort_storage_; }
  auto inverse_index() const noexcept { return inverse_index_; }
  auto instances() const noexcept { return instances_; }

  void Update(uint32_t point_count, const VrdxSorterStorageRequirements& storage_requirements);

 private:
  std::shared_ptr<gpu::Device> device_;
  uint32_t point_count_ = 0;

  // Fixed
  std::shared_ptr<gpu::Buffer> visible_point_count_;  // (1)
  std::shared_ptr<gpu::Buffer> camera_;               // (Camera)
  std::shared_ptr<gpu::Buffer> draw_indirect_;        // (DrawIndirect)
  std::shared_ptr<gpu::Buffer> camera_stage_;         // (Camera)

  // Variable
  std::shared_ptr<gpu::Buffer> key_;            // (N)
  std::shared_ptr<gpu::Buffer> index_;          // (N)
  std::shared_ptr<gpu::Buffer> sort_storage_;   // (M)
  std::shared_ptr<gpu::Buffer> inverse_index_;  // (N)
  std::shared_ptr<gpu::Buffer> instances_;      // (N, 12)
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMPUTE_STORAGE_H
