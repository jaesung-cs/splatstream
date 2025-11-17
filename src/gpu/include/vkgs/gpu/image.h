#ifndef VKGS_GPU_IMAGE_H
#define VKGS_GPU_IMAGE_H

#include <memory>
#include <cstdint>

#include <vulkan/vulkan.h>

#include "export_api.h"

#include "object.h"

namespace vkgs {
namespace gpu {

class Device;

class VKGS_GPU_API Image : public Object {
 public:
  static std::shared_ptr<Image> Create(std::shared_ptr<Device> device, VkFormat format, uint32_t width, uint32_t height,
                                       VkImageUsageFlags usage);

 public:
  Image(std::shared_ptr<Device> device, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage);
  ~Image() override;

  operator VkImage() const noexcept { return image_; }

  VkImageView image_view() const noexcept { return image_view_; }
  VkFormat format() const noexcept { return format_; }
  uint32_t width() const noexcept { return width_; }
  uint32_t height() const noexcept { return height_; }

 private:
  VkImage image_ = VK_NULL_HANDLE;
  void* allocation_ = VK_NULL_HANDLE;
  VkImageView image_view_ = VK_NULL_HANDLE;
  VkFormat format_;
  uint32_t width_;
  uint32_t height_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_IMAGE_H
