#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <memory>

#include "export_api.h"

#include "camera_params.h"

namespace vkgs {
namespace core {
class GaussianSplats;
class Renderer;
}  // namespace core

namespace viewer {

class VKGS_VIEWER_API Viewer {
 public:
  Viewer();
  ~Viewer();

  void SetRenderer(std::shared_ptr<core::Renderer> renderer);
  void SetSplats(std::shared_ptr<core::GaussianSplats> splats);

  void AddCamera(const CameraParams& camera_params);

  void Run();

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
