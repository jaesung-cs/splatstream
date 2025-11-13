#include "vkgs/gpu/gpu.h"

#include "volk.h"
#include <GLFW/glfw3.h>

namespace vkgs {
namespace gpu {

void Init() {
  volkInitialize();
  glfwInit();
}

void Terminate() {
  glfwTerminate();
  volkFinalize();
}

}  // namespace gpu
}  // namespace vkgs
