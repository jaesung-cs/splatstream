#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <array>
#include <memory>
#include <string>

#include "export_api.h"

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

  void Run();

 private:
  std::shared_ptr<Context> context_;

  GLFWwindow* window_ = nullptr;

  std::shared_ptr<core::Renderer> renderer_;
  std::shared_ptr<core::GaussianSplats> splats_;

  std::array<Storage, 2> ring_buffer_;
  uint64_t frame_index_ = 0;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
