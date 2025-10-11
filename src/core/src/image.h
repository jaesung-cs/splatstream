#ifndef VKGS_CORE_IMAGE_H
#define VKGS_CORE_IMAGE_H

#include "object.h"

#include <memory>
#include <cstdint>

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vkgs {
namespace core {

class Device;

class Image : public Object {
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
  std::shared_ptr<Device> device_;

  VkImage image_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  VkImageView image_view_ = VK_NULL_HANDLE;
  VkFormat format_;
  uint32_t width_;
  uint32_t height_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_IMAGE_H
