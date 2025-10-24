#ifndef VKGS_RENDERER_IMPL_H
#define VKGS_RENDERER_IMPL_H

#include "vkgs/renderer.h"

#include <memory>
#include <string>

#include "vkgs/core/renderer.h"

#include "vkgs/gaussian_splats.h"

namespace vkgs {

class Renderer::Impl {
 public:
  Impl();

  ~Impl();

  const std::string& device_name() const noexcept { return renderer_->device_name(); }
  uint32_t graphics_queue_index() const noexcept { return renderer_->graphics_queue_index(); }
  uint32_t compute_queue_index() const noexcept { return renderer_->compute_queue_index(); }
  uint32_t transfer_queue_index() const noexcept { return renderer_->transfer_queue_index(); }

  GaussianSplats LoadFromPly(const std::string& path);

  RenderedImage Draw(GaussianSplats splats, const float* view, const float* projection, uint32_t width, uint32_t height,
                     const float* background, float eps2d, uint8_t* dst);

 private:
  std::shared_ptr<core::Renderer> renderer_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_IMPL_H
