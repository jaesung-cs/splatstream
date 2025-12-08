#include "vkgs/engine.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vkgs/gaussian_splats.h"
#include "vkgs/rendering_task.h"

#include "vkgs/core/renderer.h"
#include "vkgs/core/parser.h"
#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/rendering_task.h"
#include "vkgs/viewer/viewer.h"

namespace vkgs {

class Engine::Impl {
 public:
  Impl() : viewer_(viewer::Viewer::Create()), parser_(core::Parser::Create()), renderer_(core::Renderer::Create()) {
    viewer_->SetRenderer(renderer_);
  }

  ~Impl() = default;

  const std::string& device_name() const noexcept { return renderer_->device_name(); }
  uint32_t graphics_queue_index() const noexcept { return renderer_->graphics_queue_index(); }
  uint32_t compute_queue_index() const noexcept { return renderer_->compute_queue_index(); }
  uint32_t transfer_queue_index() const noexcept { return renderer_->transfer_queue_index(); }

  GaussianSplats LoadFromPly(const std::string& path, int sh_degree) {
    return GaussianSplats(parser_->LoadFromPly(path, sh_degree));
  }

  GaussianSplats CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                      const float* opacities, const uint16_t* colors, int sh_degree) {
    return GaussianSplats(parser_->CreateGaussianSplats(size, means, quats, scales, opacities, colors, sh_degree));
  }

  RenderingTask Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst) {
    core::DrawOptions core_draw_options = {
        .view = glm::make_mat4(draw_options.view),
        .projection = glm::make_mat4(draw_options.projection),
        .model = glm::make_mat4(draw_options.model),
        .width = draw_options.width,
        .height = draw_options.height,
        .background = glm::make_vec3(draw_options.background),
        .eps2d = draw_options.eps2d,
        .sh_degree = draw_options.sh_degree,
        .instance_vec4 = true,
    };
    core::ScreenSplatOptions core_screen_splat_options = {
        .confidence_radius = draw_options.confidence_radius,
    };
    return RenderingTask(renderer_->Draw(splats.get(), core_draw_options, core_screen_splat_options, dst));
  }

  void AddCamera(const CameraParams& camera_params) {
    viewer::CameraParams viewer_camera_params = {
        .extrinsic = glm::make_mat4(camera_params.extrinsic),
        .intrinsic = glm::make_mat3(camera_params.intrinsic),
        .width = camera_params.width,
        .height = camera_params.height,
    };
    viewer_->AddCamera(viewer_camera_params);
  }

  void ClearCameras() { viewer_->ClearCameras(); }

  void Show(GaussianSplats splats) {
    viewer_->SetSplats(splats.get());
    viewer_->Run();
  }

 private:
  viewer::Viewer viewer_;
  core::Parser parser_;
  core::Renderer renderer_;
};

Engine::Engine() : impl_(std::make_shared<Impl>()) {}

Engine::~Engine() = default;

const std::string& Engine::device_name() const noexcept { return impl_->device_name(); }
uint32_t Engine::graphics_queue_index() const noexcept { return impl_->graphics_queue_index(); }
uint32_t Engine::compute_queue_index() const noexcept { return impl_->compute_queue_index(); }
uint32_t Engine::transfer_queue_index() const noexcept { return impl_->transfer_queue_index(); }

GaussianSplats Engine::LoadFromPly(const std::string& path, int sh_degree) {
  return impl_->LoadFromPly(path, sh_degree);
}

GaussianSplats Engine::CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                            const float* opacities, const uint16_t* colors, int sh_degree) {
  return impl_->CreateGaussianSplats(size, means, quats, scales, opacities, colors, sh_degree);
}

RenderingTask Engine::Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst) {
  return impl_->Draw(splats, draw_options, dst);
}

void Engine::AddCamera(const CameraParams& camera_params) { impl_->AddCamera(camera_params); }

void Engine::ClearCameras() { impl_->ClearCameras(); }

void Engine::Show(GaussianSplats splats) { impl_->Show(splats); }

}  // namespace vkgs
