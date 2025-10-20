#include "vkgs/gpu/image.h"

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {

std::shared_ptr<Image> Image::Create(std::shared_ptr<Device> device, VkFormat format, uint32_t width, uint32_t height,
                                     VkImageUsageFlags usage) {
  return std::make_shared<Image>(device, format, width, height, usage);
}

Image::Image(std::shared_ptr<Device> device, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage)
    : device_(device), format_(format), width_(width), height_(height) {
  VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = format_;
  image_info.extent.width = width_;
  image_info.extent.height = height_;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = usage;
  VmaAllocationCreateInfo allocation_info = {};
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
  vmaCreateImage(device_->allocator(), &image_info, &allocation_info, &image_, &allocation_, NULL);

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.image = image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format_;
  view_info.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                          VK_COMPONENT_SWIZZLE_A};
  view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  vkCreateImageView(*device_, &view_info, nullptr, &image_view_);
}

Image::~Image() {
  vkDestroyImageView(*device_, image_view_, nullptr);
  vmaDestroyImage(device_->allocator(), image_, allocation_);
}

}  // namespace gpu
}  // namespace vkgs
