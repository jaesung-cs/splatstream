#ifndef VKGS_GPU_SWAPCHAIN_H
#define VKGS_GPU_SWAPCHAIN_H

#include <array>
#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

#include "export_api.h"
#include "object.h"

namespace vkgs {
namespace gpu {

struct PresentImageInfo {
  uint32_t image_index;
  VkImage image;
  VkImageView image_view;
  VkExtent2D extent;
  VkSemaphore image_available_semaphore;
  VkSemaphore render_finished_semaphore;
};

class VKGS_GPU_API SwapchainImpl : public Object {
 public:
  SwapchainImpl(VkSurfaceKHR surface, VkFormat format, VkImageUsageFlags usage);
  ~SwapchainImpl() override;

  VkFormat format() const noexcept { return format_; }

  void SetPresentMode(VkPresentModeKHR present_mode);

  PresentImageInfo AcquireNextImage();
  void Present();

  void Wait() const;

 private:
  void GetDefaultSwapchainCreateInfo(VkSwapchainCreateInfoKHR* swapchain_info);
  void Recreate();

  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat format_ = VK_FORMAT_B8G8R8A8_UNORM;
  VkImageUsageFlags usage_ = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  VkPresentModeKHR present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
  VkExtent2D extent_ = {0, 0};

  bool need_recreate_ = false;

  // Present
  static constexpr uint32_t kFrameCount = 2;
  uint32_t image_index_ = 0;
  uint32_t frame_index_ = 0;
  std::array<VkImage, 3> images_;
  std::array<VkImageView, 3> image_views_ = {VK_NULL_HANDLE};
  std::array<VkSemaphore, kFrameCount> image_available_semaphores_;
  std::array<VkSemaphore, kFrameCount> render_finished_semaphores_;
  std::array<VkFence, kFrameCount> render_finished_fences_;
};

class VKGS_GPU_API Swapchain : public SharedAccessor<Swapchain, SwapchainImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SWAPCHAIN_H