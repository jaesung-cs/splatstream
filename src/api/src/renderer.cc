#include "vkgs/renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vkgs/gaussian_splats.h"
#include "vkgs/rendering_task.h"

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

RenderingTask Renderer::Draw(GaussianSplats splats, const std::vector<DrawOptions>& draw_options, uint32_t width,
                             uint32_t height, uint8_t* dst) {
  std::vector<core::DrawOptions> core_draw_options;
  core_draw_options.resize(draw_options.size());
  for (size_t i = 0; i < draw_options.size(); ++i) {
    core_draw_options[i].view = glm::make_mat4(draw_options[i].view);
    core_draw_options[i].projection = glm::make_mat4(draw_options[i].projection);
    core_draw_options[i].background = glm::make_vec3(draw_options[i].background);
    core_draw_options[i].eps2d = draw_options[i].eps2d;
    core_draw_options[i].sh_degree = draw_options[i].sh_degree;
  }
  return RenderingTask(renderer_->Draw(splats.get(), core_draw_options, width, height, dst));
}

}  // namespace vkgs
