#ifndef VKGS_RENDERER_H
#define VKGS_RENDERER_H

#include <memory>
#include <string>

#include "vkgs/export_api.h"

namespace vkgs {

class GaussianSplats;
class RenderedImage;

class VKGS_API Renderer {
 public:
  Renderer();
  ~Renderer();

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  GaussianSplats LoadFromPly(const std::string& path);

  /**
   * view: 4x4 column-major matrix.
   * projection: 4x4 column-major matrix.
   * background: 3 components rgb.
   * dst: (height, width, 4) uint8 image, row-major.
   */
  RenderedImage Draw(GaussianSplats splats, const float* view, const float* projection, uint32_t width, uint32_t height,
                     const float* background, float eps2d, uint8_t* dst);

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_H
