#include "vkgs/renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vkgs/gaussian_splats.h"
#include "vkgs/rendered_image.h"

#include "vkgs/core/draw_options.h"
#include "vkgs/core/renderer.h"

namespace vkgs {

Renderer::Renderer() : renderer_(std::make_shared<core::Renderer>()) {}

Renderer::~Renderer() = default;

const std::string& Renderer::device_name() const noexcept { return renderer_->device_name(); }
uint32_t Renderer::graphics_queue_index() const noexcept { return renderer_->graphics_queue_index(); }
uint32_t Renderer::compute_queue_index() const noexcept { return renderer_->compute_queue_index(); }
uint32_t Renderer::transfer_queue_index() const noexcept { return renderer_->transfer_queue_index(); }

GaussianSplats Renderer::LoadFromPly(const std::string& path, int sh_degree) {
  return GaussianSplats(renderer_->LoadFromPly(path, sh_degree));
}

GaussianSplats Renderer::CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                              const float* opacities, const uint16_t* colors, int sh_degree) {
  return GaussianSplats(renderer_->CreateGaussianSplats(size, means, quats, scales, opacities, colors, sh_degree));
}

RenderedImage Renderer::Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst) {
  core::DrawOptions core_draw_options = {};
  core_draw_options.view = glm::make_mat4(draw_options.view);
  core_draw_options.projection = glm::make_mat4(draw_options.projection);
  core_draw_options.width = draw_options.width;
  core_draw_options.height = draw_options.height;
  core_draw_options.background = glm::make_vec3(draw_options.background);
  core_draw_options.eps2d = draw_options.eps2d;
  core_draw_options.sh_degree = draw_options.sh_degree;
  return RenderedImage(renderer_->Draw(splats.get(), core_draw_options, dst));
}

}  // namespace vkgs
