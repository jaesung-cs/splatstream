#ifndef VKGS_GPU_SWAPCHAIN_H
#define VKGS_GPU_SWAPCHAIN_H

#include <array>
#include <memory>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;

struct PresentImageInfo {
  uint32_t image_index;
  VkImage image;
  VkImageView image_view;
  VkExtent2D extent;
  VkSemaphore image_available_semaphore;
  VkSemaphore render_finished_semaphore;
};

class VKGS_GPU_API Swapchain {
 public:
  Swapchain(std::shared_ptr<Device> device, VkSurfaceKHR surface);
  ~Swapchain();

  VkFormat format() const noexcept { return format_; }

  PresentImageInfo AcquireNextImage();
  void Present();

  void Wait() const;

 private:
  void GetDefaultSwapchainCreateInfo(VkSwapchainCreateInfoKHR* swapchain_info);
  void Recreate();

  std::shared_ptr<Device> device_;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D extent_ = {0, 0};

  // Present
  uint32_t image_index_ = 0;
  uint32_t frame_index_ = 0;
  std::array<VkImage, 3> images_;
  std::array<VkImageView, 3> image_views_;
  std::array<VkSemaphore, 3> image_available_semaphores_;
  std::array<VkSemaphore, 3> render_finished_semaphores_;
  std::array<VkFence, 3> render_finished_fences_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SWAPCHAIN_H