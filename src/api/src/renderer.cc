#include "vkgs/renderer.h"

#include "vkgs/gaussian_splats.h"
#include "vkgs/rendered_image.h"

#include "renderer_impl.h"

namespace vkgs {

Renderer::Renderer() : impl_(std::make_shared<Impl>()) {}

Renderer::~Renderer() = default;

const std::string& Renderer::device_name() const noexcept { return impl_->device_name(); }
uint32_t Renderer::graphics_queue_index() const noexcept { return impl_->graphics_queue_index(); }
uint32_t Renderer::compute_queue_index() const noexcept { return impl_->compute_queue_index(); }
uint32_t Renderer::transfer_queue_index() const noexcept { return impl_->transfer_queue_index(); }

GaussianSplats Renderer::LoadFromPly(const std::string& path) { return impl_->LoadFromPly(path); }

RenderedImage Renderer::Draw(GaussianSplats splats, const float* view, const float* projection, uint32_t width,
                             uint32_t height, const float* background, float eps2d, uint8_t* dst) {
  return impl_->Draw(splats, view, projection, width, height, background, eps2d, dst);
}

}  // namespace vkgs
