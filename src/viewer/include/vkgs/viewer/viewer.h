#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <memory>

#include "vkgs/common/shared_accessor.h"

#include "vkgs/viewer/export_api.h"
#include "vkgs/viewer/camera_params.h"

namespace vkgs {
namespace core {
class GaussianSplats;
class Renderer;
}  // namespace core

namespace viewer {

class VKGS_VIEWER_API ViewerImpl {
 public:
  ViewerImpl();
  ~ViewerImpl();

  void SetRenderer(core::Renderer renderer);
  void SetSplats(core::GaussianSplats splats);

  void AddCamera(const CameraParams& camera_params);
  void ClearCameras();

  void Run();

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

class Viewer : public SharedAccessor<Viewer, ViewerImpl> {};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
