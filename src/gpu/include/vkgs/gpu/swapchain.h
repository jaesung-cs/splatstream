#ifndef VKGS_GPU_SWAPCHAIN_H
#define VKGS_GPU_SWAPCHAIN_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

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

class SwapchainImpl;
class VKGS_GPU_API Swapchain : public Handle<Swapchain, SwapchainImpl> {
 public:
  static Swapchain Create(VkSurfaceKHR surface, VkFormat format, VkImageUsageFlags usage);

  VkFormat format() const;
  void SetPresentMode(VkPresentModeKHR present_mode);
  PresentImageInfo AcquireNextImage();
  void Present();
  void Wait() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SWAPCHAIN_H