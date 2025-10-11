#include "module_impl.h"

#include "vkgs/gaussian_splats.h"

#include "vkgs/core/module.h"

#include "gaussian_splats_impl.h"
#include "rendered_image_impl.h"

namespace vkgs {

Module::Impl::Impl() { module_ = std::make_shared<core::Module>(); }

Module::Impl::~Impl() = default;

GaussianSplats Module::Impl::load_from_ply(const std::string& path) {
  GaussianSplats gaussian_splats;
  gaussian_splats.impl()->SetGaussianSplats(module_->load_from_ply(path));
  return gaussian_splats;
}

RenderedImage Module::Impl::draw(GaussianSplats splats) {
  RenderedImage rendered_image;
  rendered_image.impl()->SetRenderedImage(module_->draw(splats.impl()->GetGaussianSplats()));
  return rendered_image;
}

}  // namespace vkgs
