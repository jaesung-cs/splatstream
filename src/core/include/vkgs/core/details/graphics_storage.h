#ifndef VKGS_CORE_DETAILS_GRAPHICS_STORAGE_H
#define VKGS_CORE_DETAILS_GRAPHICS_STORAGE_H

#include <memory>
#include <cstdint>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/image.h"

namespace vkgs {
namespace core {

class GraphicsStorageImpl {
 public:
  GraphicsStorageImpl();
  ~GraphicsStorageImpl();

  auto image() const noexcept { return image_; }
  auto image_u8() const noexcept { return image_u8_; }

  void Update(uint32_t width, uint32_t height);

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
  gpu::Image image_;     // (H, W, 4) float32
  gpu::Image image_u8_;  // (H, W, 4), UNORM
};

class GraphicsStorage : public SharedAccessor<GraphicsStorage, GraphicsStorageImpl> {};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DETAILS_GRAPHICS_STORAGE_H
