#include "context.h"

#include <GLFW/glfw3.h>

#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/device.h"

namespace vkgs {
namespace viewer {
namespace {

Context::Weak global_context;

}

Context GetContext() {
  if (auto handle = global_context.lock()) return handle;
  auto handle = Context::Create();
  global_context = handle;
  return handle;
}

class ContextImpl {
 public:
  void __init__() {
    glfwInit();

    // GLFW required instance extensions
    uint32_t count;
    const char** instance_extensions = glfwGetRequiredInstanceExtensions(&count);
    device_ = gpu::GetDevice({
        .enable_viewer = true,
        .instance_extensions = {instance_extensions, instance_extensions + count},
    });
  }

  void __del__() { glfwTerminate(); }

  auto device() const noexcept { return device_; }

 private:
  gpu::Device device_;
};

Context Context::Create() { return Make<ContextImpl>(); }

gpu::Device Context::device() const noexcept { return impl_->device(); }

}  // namespace viewer
}  // namespace vkgs
