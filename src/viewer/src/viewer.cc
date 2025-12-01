#include "vkgs/viewer/viewer.h"

#include "viewer_impl.h"

namespace vkgs {
namespace viewer {

Viewer::Viewer() { impl_ = std::make_shared<Impl>(); }
Viewer::~Viewer() = default;

void Viewer::SetRenderer(std::shared_ptr<core::Renderer> renderer) { impl_->SetRenderer(renderer); }
void Viewer::SetSplats(core::GaussianSplats splats) { impl_->SetSplats(splats); }
void Viewer::AddCamera(const CameraParams& camera_params) { impl_->AddCamera(camera_params); }
void Viewer::Run() { impl_->Run(); }

}  // namespace viewer
}  // namespace vkgs
