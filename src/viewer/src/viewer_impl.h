#ifndef VKGS_VIEWER_VIEWER_IMPL_H
#define VKGS_VIEWER_VIEWER_IMPL_H

#include <array>
#include <vector>

#include "vkgs/viewer/viewer.h"

#include "storage.h"

struct GLFWwindow;

namespace vkgs {
namespace viewer {

class Context;

class Viewer::Impl {
 public:
  Impl();
  ~Impl();

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

#endif  // VKGS_VIEWER_VIEWER_IMPL_H
