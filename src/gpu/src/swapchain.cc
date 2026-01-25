#include "vkgs/gpu/swapchain.h"

#include <array>

#include "volk.h"

#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class SwapchainImpl : public Object {
 public:
  void __init__(VkSurfaceKHR surface, VkFormat format, VkImageUsageFlags usage) {
    surface_ = surface;
    format_ = format;
    usage_ = usage;

    need_recreate_ = true;

    for (int i = 0; i < kFrameCount; ++i) {
      VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      vkCreateSemaphore(device_, &semaphore_info, NULL, &render_finished_semaphores_[i]);
      vkCreateSemaphore(device_, &semaphore_info, NULL, &image_available_semaphores_[i]);

      VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      vkCreateFence(device_, &fence_info, NULL, &render_finished_fences_[i]);
    }
  }

  void __del__() {
    if (swapchain_) {
      Wait();

      for (int i = 0; i < 3; ++i) {
        vkDestroyImageView(device_, image_views_[i], NULL);
      }

      if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, NULL);
      vkDestroySurfaceKHR(device_.instance(), surface_, NULL);
    }

    for (int i = 0; i < kFrameCount; ++i) {
      vkDestroySemaphore(device_, image_available_semaphores_[i], NULL);
      vkDestroySemaphore(device_, render_finished_semaphores_[i], NULL);
      vkDestroyFence(device_, render_finished_fences_[i], NULL);
    }
  }

  VkFormat format() const noexcept { return format_; }

  void SetPresentMode(VkPresentModeKHR present_mode) {
    present_mode_ = present_mode;
    need_recreate_ = true;
  }

  PresentImageInfo AcquireNextImage() {
    uint32_t image_index = 0;
    VkFence render_finished_fence = render_finished_fences_[frame_index_];
    vkWaitForFences(device_, 1, &render_finished_fence, VK_TRUE, UINT64_MAX);

    VkSemaphore image_available_semaphore = image_available_semaphores_[frame_index_];
    VkSemaphore render_finished_semaphore = render_finished_semaphores_[frame_index_];

    if (need_recreate_) Recreate();

    while (true) {
      VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_semaphore,
                                              VK_NULL_HANDLE, &image_index);

      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        Recreate();
      } else {
        break;
      }
    }

    image_index_ = image_index;

    PresentImageInfo present_image_info;
    present_image_info.image_index = image_index;
    present_image_info.image = images_[image_index];
    present_image_info.image_view = image_views_[image_index];
    present_image_info.extent = extent_;
    present_image_info.image_available_semaphore = image_available_semaphore;
    present_image_info.render_finished_semaphore = render_finished_semaphore;
    return present_image_info;
  }

  void Present() {
    VkSemaphore render_finished_semaphore = render_finished_semaphores_[frame_index_];
    VkFence render_finished_fence = render_finished_fences_[frame_index_];

    vkResetFences(device_, 1, &render_finished_fence);

    VkSwapchainPresentFenceInfoKHR fence_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR};
    fence_info.swapchainCount = 1;
    fence_info.pFences = &render_finished_fence;

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.pNext = &fence_info;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &image_index_;
    vkQueuePresentKHR(device_.graphics_queue(), &present_info);

    frame_index_ = (frame_index_ + 1) % kFrameCount;
  }

  void Wait() const { vkWaitForFences(device_, kFrameCount, render_finished_fences_.data(), VK_TRUE, UINT64_MAX); }

 private:
  void GetDefaultSwapchainCreateInfo(VkSwapchainCreateInfoKHR* swapchain_info) {
    auto physical_device = device_.physical_device();

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface_, &surface_capabilities);

    *swapchain_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info->surface = surface_;
    swapchain_info->minImageCount = 3;
    swapchain_info->imageFormat = format_;
    swapchain_info->imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info->imageExtent = surface_capabilities.currentExtent;
    swapchain_info->imageArrayLayers = 1;
    swapchain_info->imageUsage = usage_;
    swapchain_info->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info->preTransform = surface_capabilities.currentTransform;
    swapchain_info->presentMode = present_mode_;
    swapchain_info->clipped = VK_TRUE;
  }

  void Recreate() {
    Wait();

    VkSwapchainCreateInfoKHR swapchain_info;
    GetDefaultSwapchainCreateInfo(&swapchain_info);
    swapchain_info.oldSwapchain = swapchain_;

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device_, &swapchain_info, NULL, &swapchain);
    vkDestroySwapchainKHR(device_, swapchain_, NULL);
    swapchain_ = swapchain;
    extent_ = swapchain_info.imageExtent;

    uint32_t image_count = 3;
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, images_.data());

    for (int i = 0; i < 3; ++i) {
      if (image_views_[i]) vkDestroyImageView(device_, image_views_[i], NULL);

      VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      view_info.image = images_[i];
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = format_;
      view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCreateImageView(device_, &view_info, NULL, &image_views_[i]);
    }

    need_recreate_ = false;
  }

  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat format_ = VK_FORMAT_B8G8R8A8_UNORM;
  VkImageUsageFlags usage_ = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

Swapchain Swapchain::Create(VkSurfaceKHR surface, VkFormat format, VkImageUsageFlags usage) {
  return Make<SwapchainImpl>(surface, format, usage);
}

VkFormat Swapchain::format() const { return impl_->format(); }
void Swapchain::SetPresentMode(VkPresentModeKHR present_mode) { impl_->SetPresentMode(present_mode); }
PresentImageInfo Swapchain::AcquireNextImage() { return impl_->AcquireNextImage(); }
void Swapchain::Present() { impl_->Present(); }
void Swapchain::Wait() const { impl_->Wait(); }

}  // namespace gpu
}  // namespace vkgs
