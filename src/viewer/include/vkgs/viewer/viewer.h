#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <memory>

#include "vkgs/common/handle.h"

#include "vkgs/viewer/export_api.h"
#include "vkgs/viewer/camera_params.h"

namespace vkgs {
namespace core {
class GaussianSplats;
class Renderer;
}  // namespace core

namespace viewer {

class ViewerImpl;
class VKGS_VIEWER_API Viewer : public Handle<Viewer, ViewerImpl> {
 public:
  static Viewer Create();

  void SetRenderer(core::Renderer renderer);
  void SetSplats(core::GaussianSplats splats);

  void AddCamera(const CameraParams& camera_params);
  void ClearCameras();

  void Run();
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
