#include "vkgs/engine.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vkgs/gaussian_splats.h"
#include "vkgs/rendering_task.h"

#include "vkgs/core/draw_options.h"
#include "vkgs/core/renderer.h"
#include "vkgs/core/parser.h"
#include "vkgs/viewer/viewer.h"

namespace vkgs {

Engine::Engine()
    : viewer_(std::make_shared<viewer::Viewer>()),
      parser_(std::make_shared<core::Parser>()),
      renderer_(std::make_shared<core::Renderer>()) {
  viewer_->SetRenderer(renderer_);
}

Engine::~Engine() = default;

const std::string& Engine::device_name() const noexcept { return renderer_->device_name(); }
uint32_t Engine::graphics_queue_index() const noexcept { return renderer_->graphics_queue_index(); }
uint32_t Engine::compute_queue_index() const noexcept { return renderer_->compute_queue_index(); }
uint32_t Engine::transfer_queue_index() const noexcept { return renderer_->transfer_queue_index(); }

GaussianSplats Engine::LoadFromPly(const std::string& path, int sh_degree) {
  return GaussianSplats(parser_->LoadFromPly(path, sh_degree));
}

GaussianSplats Engine::CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                            const float* opacities, const uint16_t* colors, int sh_degree) {
  return GaussianSplats(parser_->CreateGaussianSplats(size, means, quats, scales, opacities, colors, sh_degree));
}

RenderingTask Engine::Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst) {
  core::DrawOptions core_draw_options = {};
  core_draw_options.view = glm::make_mat4(draw_options.view);
  core_draw_options.projection = glm::make_mat4(draw_options.projection);
  core_draw_options.model = glm::make_mat4(draw_options.model);
  core_draw_options.width = draw_options.width;
  core_draw_options.height = draw_options.height;
  core_draw_options.background = glm::make_vec3(draw_options.background);
  core_draw_options.eps2d = draw_options.eps2d;
  core_draw_options.sh_degree = draw_options.sh_degree;
  return RenderingTask(renderer_->Draw(splats.get(), core_draw_options, dst));
}

void Engine::AddCamera(const CameraParams& camera_params) {
  viewer::CameraParams viewer_camera_params = {};
  viewer_camera_params.extrinsic = glm::make_mat4(camera_params.extrinsic);
  viewer_camera_params.intrinsic = glm::make_mat3(camera_params.intrinsic);
  viewer_camera_params.width = camera_params.width;
  viewer_camera_params.height = camera_params.height;
  viewer_->AddCamera(viewer_camera_params);
}

void Engine::Show(GaussianSplats splats) {
  viewer_->SetSplats(splats.get());
  viewer_->Run();
}

}  // namespace vkgs
