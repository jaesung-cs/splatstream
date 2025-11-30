#ifndef VKGS_CORE_SCREEN_SPLATS_H
#define VKGS_CORE_SCREEN_SPLATS_H

#include <memory>
#include <cstdint>

#include "export_api.h"

#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

class VKGS_CORE_API ScreenSplats {
 public:
  ScreenSplats();
  ~ScreenSplats();

  auto draw_indirect() const noexcept { return draw_indirect_; }
  auto instances() const noexcept { return instances_; }

  void Update(uint32_t point_count);

 private:
  uint32_t point_count_ = 0;

  // Fixed
  gpu::Buffer draw_indirect_;  // (DrawIndirect)

  // Variable
  gpu::Buffer instances_;  // (N, 12)
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMPUTE_STORAGE_H
