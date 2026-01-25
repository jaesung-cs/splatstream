#ifndef VKGS_CORE_SCREEN_SPLATS_H
#define VKGS_CORE_SCREEN_SPLATS_H

#include <glm/fwd.hpp>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/fwd.h"
#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class ScreenSplatsImpl;
class VKGS_CORE_API ScreenSplats : public Handle<ScreenSplats, ScreenSplatsImpl> {
 public:
  static ScreenSplats Create();

  void SetProjection(const glm::mat4& projection);
  void SetIndexBuffer(gpu::Buffer index_buffer);

  gpu::Buffer visible_point_count() const;
  glm::mat4 projection() const;
  gpu::Buffer draw_indirect() const;
  gpu::Buffer instances() const;
  gpu::Buffer index_buffer() const;
  gpu::Buffer stats() const;

  // Internal
  void Update(uint32_t point_count);
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMPUTE_STORAGE_H
