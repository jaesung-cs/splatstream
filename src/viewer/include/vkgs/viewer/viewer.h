#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <array>
#include <memory>
#include <string>

#include "export_api.h"

#include "storage.h"

struct GLFWwindow;

namespace vkgs {
namespace viewer {

class Context;

class VKGS_VIEWER_API Viewer {
 public:
  Viewer();
  ~Viewer();

  void SetModelPath(const std::string& path) { model_path_ = path; }
  void Run();

 private:
  std::shared_ptr<Context> context_;

  GLFWwindow* window_ = nullptr;

  std::string model_path_;

  std::array<Storage, 2> ring_buffer_;
  uint64_t frame_index_ = 0;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
