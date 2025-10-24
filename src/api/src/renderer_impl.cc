#include "renderer_impl.h"

#include "vkgs/gaussian_splats.h"

#include "vkgs/core/renderer.h"

#include "gaussian_splats_impl.h"
#include "rendered_image_impl.h"

namespace vkgs {

Renderer::Impl::Impl() { renderer_ = std::make_shared<core::Renderer>(); }

Renderer::Impl::~Impl() = default;

GaussianSplats Renderer::Impl::LoadFromPly(const std::string& path, int sh_degree) {
  GaussianSplats gaussian_splats;
  gaussian_splats.impl()->SetGaussianSplats(renderer_->LoadFromPly(path, sh_degree));
  return gaussian_splats;
}

RenderedImage Renderer::Impl::Draw(GaussianSplats splats, const core::DrawOptions& draw_options, uint8_t* dst) {
  RenderedImage rendered_image;
  rendered_image.impl()->SetRenderedImage(renderer_->Draw(splats.impl()->GetGaussianSplats(), draw_options, dst));
  return rendered_image;
}

}  // namespace vkgs
