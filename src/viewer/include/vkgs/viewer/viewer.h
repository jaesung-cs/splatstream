#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "export_api.h"

#include "camera_params.h"
#include "storage.h"

struct GLFWwindow;

namespace vkgs {
namespace core {
class GaussianSplats;
class Renderer;
}  // namespace core

namespace viewer {

class Context;

class VKGS_VIEWER_API Viewer {
 public:
  Viewer();
  ~Viewer();

  void SetRenderer(std::shared_ptr<core::Renderer> renderer) { renderer_ = renderer; }
  void SetSplats(std::shared_ptr<core::GaussianSplats> splats) { splats_ = splats; }

  void AddCamera(const CameraParams& camera_params) { camera_params_.push_back(camera_params); }

  void Run();

 private:
  std::shared_ptr<Context> context_;

  GLFWwindow* window_ = nullptr;

  std::shared_ptr<core::Renderer> renderer_;
  std::shared_ptr<core::GaussianSplats> splats_;
  std::vector<CameraParams> camera_params_;

  std::array<Storage, 2> ring_buffer_;
  uint64_t frame_index_ = 0;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
