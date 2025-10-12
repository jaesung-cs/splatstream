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

RenderedImage Module::Impl::draw(GaussianSplats splats, const float* view, const float* projection, uint32_t width,
                                 uint32_t height) {
  glm::mat4 view_mat;
  glm::mat4 projection_mat;

  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      view_mat[c][r] = view[c * 4 + r];
      projection_mat[c][r] = projection[c * 4 + r];
    }
  }

  RenderedImage rendered_image;
  rendered_image.impl()->SetRenderedImage(
      module_->draw(splats.impl()->GetGaussianSplats(), view_mat, projection_mat, width, height));
  return rendered_image;
}

}  // namespace vkgs
