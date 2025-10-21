#ifndef VKGS_CORE_RENDERED_IMAGE_H
#define VKGS_CORE_RENDERED_IMAGE_H

#include <vector>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Buffer;
class Fence;

}  // namespace gpu

namespace core {

class VKGS_CORE_API RenderedImage {
 public:
  RenderedImage(uint32_t width, uint32_t height, std::shared_ptr<gpu::Fence> fence,
                std::shared_ptr<gpu::Buffer> image_buffer, uint8_t* dst);
  ~RenderedImage();

  uint32_t width() const noexcept { return width_; }
  uint32_t height() const noexcept { return height_; }

  void Wait();

 private:
  uint32_t width_;
  uint32_t height_;
  std::shared_ptr<gpu::Fence> fence_;
  std::shared_ptr<gpu::Buffer> image_buffer_;
  uint8_t* dst_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERED_IMAGE_H
