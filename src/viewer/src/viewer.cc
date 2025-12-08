#include "vkgs/viewer/viewer.h"

#include "vkgs/core/renderer.h"

#include "viewer_impl.h"

namespace vkgs {
namespace viewer {

ViewerImpl::ViewerImpl() { impl_ = std::make_shared<Impl>(); }
ViewerImpl::~ViewerImpl() = default;

void ViewerImpl::SetRenderer(core::Renderer renderer) { impl_->SetRenderer(renderer); }
void ViewerImpl::SetSplats(core::GaussianSplats splats) { impl_->SetSplats(splats); }
void ViewerImpl::AddCamera(const CameraParams& camera_params) { impl_->AddCamera(camera_params); }
void ViewerImpl::ClearCameras() { impl_->ClearCameras(); }
void ViewerImpl::Run() { impl_->Run(); }

}  // namespace viewer
}  // namespace vkgs
