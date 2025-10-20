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

  GaussianSplats load_from_ply(const std::string& path);

  /**
   * view: 4x4 column-major matrix.
   * projection: 4x4 column-major matrix.
   */
  RenderedImage draw(GaussianSplats splats, const float* view, const float* projection, uint32_t width,
                     uint32_t height);

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_H
