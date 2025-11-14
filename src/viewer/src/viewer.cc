#include "vkgs/viewer/viewer.h"

#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/gpu.h"

namespace vkgs {
namespace viewer {

void Init() {
  glfwInit();

  // GLFW required instance extensions
  uint32_t count;
  const char** instance_extensions = glfwGetRequiredInstanceExtensions(&count);

  gpu::DeviceCreateInfo device_info;
  device_info.instance_extensions.assign(instance_extensions, instance_extensions + count);
  gpu::Init();
}

void Terminate() {
  gpu::Terminate();
  glfwTerminate();
}

Viewer::Viewer() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(1280, 720, "vkgs", nullptr, nullptr);
  if (!window_) throw std::runtime_error("Failed to create window");

  auto device = gpu::GetDevice();

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkResult result = glfwCreateWindowSurface(device->instance(), window_, NULL, &surface);
  std::cout << "result: " << result << std::endl;
  std::cout << "surface: " << surface << std::endl;
}

Viewer::~Viewer() = default;

void Viewer::Run() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
  }
}

}  // namespace viewer
}  // namespace vkgs
