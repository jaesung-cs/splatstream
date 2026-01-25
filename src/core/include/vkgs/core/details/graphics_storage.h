#ifndef VKGS_CORE_DETAILS_GRAPHICS_STORAGE_H
#define VKGS_CORE_DETAILS_GRAPHICS_STORAGE_H

#include <memory>
#include <cstdint>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/image.h"

namespace vkgs {
namespace core {

class GraphicsStorageImpl;
class GraphicsStorage : public Handle<GraphicsStorage, GraphicsStorageImpl> {
 public:
  static GraphicsStorage Create();

  gpu::Image image() const;
  gpu::Image image_u8() const;

  void Update(uint32_t width, uint32_t height);
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DETAILS_GRAPHICS_STORAGE_H
