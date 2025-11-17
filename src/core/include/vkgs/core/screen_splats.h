#ifndef VKGS_CORE_SCREEN_SPLATS_H
#define VKGS_CORE_SCREEN_SPLATS_H

#include <memory>
#include <cstdint>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;
class Buffer;

}  // namespace gpu

namespace core {

class VKGS_CORE_API ScreenSplats {
 public:
  ScreenSplats(std::shared_ptr<gpu::Device> device);
  ~ScreenSplats();

  auto draw_indirect() const noexcept { return draw_indirect_; }
  auto instances() const noexcept { return instances_; }

  void Update(uint32_t point_count);

 private:
  std::shared_ptr<gpu::Device> device_;
  uint32_t point_count_ = 0;

  // Fixed
  std::shared_ptr<gpu::Buffer> draw_indirect_;  // (DrawIndirect)

  // Variable
  std::shared_ptr<gpu::Buffer> instances_;  // (N, 12)
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMPUTE_STORAGE_H
