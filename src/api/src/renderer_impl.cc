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

RenderedImage Renderer::Impl::Draw(GaussianSplats splats, const float* view, const float* projection, uint32_t width,
                                   uint32_t height, const float* background, float eps2d, int sh_degree, uint8_t* dst) {
  glm::mat4 view_mat;
  glm::mat4 projection_mat;
  glm::vec3 background_vec = glm::vec3(background[0], background[1], background[2]);

  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      view_mat[c][r] = view[c * 4 + r];
      projection_mat[c][r] = projection[c * 4 + r];
    }
  }

  RenderedImage rendered_image;
  rendered_image.impl()->SetRenderedImage(renderer_->Draw(splats.impl()->GetGaussianSplats(), view_mat, projection_mat,
                                                          width, height, background_vec, eps2d, sh_degree, dst));
  return rendered_image;
}

}  // namespace vkgs
