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

  gpu::Init({
      .enable_viewer = true,
      .instance_extensions = {instance_extensions, instance_extensions + count},
  });
  device_ = gpu::GetDevice();
}

Context::~Context() { glfwTerminate(); }

}  // namespace viewer
}  // namespace vkgs
