#include "vkgs/viewer/viewer.h"

#include <stdexcept>

#include "volk.h"
#include <GLFW/glfw3.h>

namespace vkgs {
namespace viewer {

Viewer::Viewer() {
  if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
  if (!glfwVulkanSupported()) throw std::runtime_error("Failed to initialize GLFW with Vulkan");
  glfwInitVulkanLoader(vkGetInstanceProcAddr);

  // GLFW required instance extensions
  uint32_t count;
  const char** instance_extensions = glfwGetRequiredInstanceExtensions(&count);

  // glfwCreateWindowSurface(instance, window, NULL, &surface);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(1280, 720, "vkgs", nullptr, nullptr);
  if (!window_) throw std::runtime_error("Failed to create window");
}

Viewer::~Viewer() { glfwTerminate(); }

void Viewer::Run() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
  }
}

}  // namespace viewer
}  // namespace vkgs
