#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include "export_api.h"

struct GLFWwindow;

namespace vkgs {
namespace viewer {

class VKGS_VIEWER_API Viewer {
 public:
  Viewer();
  ~Viewer();

  void Run();

 private:
  GLFWwindow* window_ = nullptr;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
