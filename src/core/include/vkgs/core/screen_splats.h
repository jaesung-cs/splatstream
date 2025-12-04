#ifndef VKGS_CORE_SCREEN_SPLATS_H
#define VKGS_CORE_SCREEN_SPLATS_H

#include <memory>
#include <cstdint>

#include "glm/glm.hpp"

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/buffer.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class VKGS_CORE_API ScreenSplatsImpl {
 public:
  ScreenSplatsImpl();
  ~ScreenSplatsImpl();

  void SetProjection(const glm::mat4& projection) { projection_ = projection; }
  void SetIndexBuffer(gpu::Buffer index_buffer) { index_buffer_ = index_buffer; }
  void SetInstanceVec4(bool instance_vec4) { instance_vec4_ = instance_vec4; }

  auto projection() const noexcept { return projection_; }
  auto draw_indirect() const noexcept { return draw_indirect_; }
  auto instances() const noexcept { return instances_; }
  auto index_buffer() const noexcept { return index_buffer_; }
  bool instance_vec4() const noexcept { return instance_vec4_; }

  // Internal
  void Update(uint32_t point_count);

 private:
  uint32_t point_count_ = 0;
  glm::mat4 projection_;
  bool instance_vec4_;

  // Fixed
  gpu::Buffer draw_indirect_;  // (DrawIndirect)

  // Variable
  gpu::Buffer instances_;  // (N, 12)

  // Shared
  gpu::Buffer index_buffer_;  // (6N)
};

class VKGS_CORE_API ScreenSplats : public SharedAccessor<ScreenSplats, ScreenSplatsImpl> {};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMPUTE_STORAGE_H
