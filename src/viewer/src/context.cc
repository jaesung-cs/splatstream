#include "context.h"

#include <GLFW/glfw3.h>

#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/device.h"

namespace vkgs {
namespace viewer {
namespace {

std::weak_ptr<Context> context;

}

std::shared_ptr<Context> GetContext() {
  if (auto ptr = context.lock()) return ptr;
  auto ptr = std::make_shared<Context>();
  context = ptr;
  return ptr;
}

Context::Context() {
  glfwInit();

  // GLFW required instance extensions
  uint32_t count;
  const char** instance_extensions = glfwGetRequiredInstanceExtensions(&count);

  gpu::DeviceCreateInfo device_info;
  device_info.enable_viewer = true;
  device_info.instance_extensions.assign(instance_extensions, instance_extensions + count);
  gpu::Init(device_info);
}

Context::~Context() { glfwTerminate(); }

}  // namespace viewer
}  // namespace vkgs
