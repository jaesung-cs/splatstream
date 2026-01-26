#ifndef VKGS_CORE_DETAILS_COMPUTE_STORAGE_H
#define VKGS_CORE_DETAILS_COMPUTE_STORAGE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/fwd.h"

namespace vkgs {
namespace core {

class ComputeStorageImpl;
class ComputeStorage : public Handle<ComputeStorage, ComputeStorageImpl> {
 public:
  static ComputeStorage Create();

  gpu::Buffer camera() const;
  gpu::Buffer camera_stage() const;
  gpu::Buffer key() const;
  gpu::Buffer index() const;
  gpu::Buffer sort_storage() const;
  gpu::Buffer inverse_index() const;

  void Update(uint32_t point_count, VkBufferUsageFlags usage, VkDeviceSize size);
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DETAILS_COMPUTE_STORAGE_H
