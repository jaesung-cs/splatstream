#ifndef VKGS_VIEWER_VIEWER_BASE_H
#define VKGS_VIEWER_VIEWER_BASE_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vkgs/gpu/swapchain.h"

#include "context.h"

namespace vkgs {
namespace viewer {

class ViewerBase {
 public:
  void __init__();
  void __del__();

  virtual void OnBeforeRun() = 0;
  virtual void OnAfterRun() = 0;

  void Run();

 private:
  virtual void DrawUi();
  virtual void Draw(const gpu::PresentImageInfo& present);

  void InitializeWindow();
  void FinalizeWindow();

 protected:
  gpu::Device device() const;

  gpu::Swapchain swapchain_;

 private:
  Context context_;
  GLFWwindow* window_ = nullptr;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_UNORM;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_BASE_H
