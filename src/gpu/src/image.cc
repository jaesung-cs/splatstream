#include "vkgs/gpu/image.h"

#include <volk.h>
#include <vk_mem_alloc.h>

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {

ImageImpl::ImageImpl(VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage)
    : format_(format), width_(width), height_(height) {
  bool is_depth = false;
  switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT:
      is_depth = true;
      break;
  }

  VmaAllocator allocator = static_cast<VmaAllocator>(device_->allocator());
  VmaAllocation allocation = VK_NULL_HANDLE;

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
  vmaCreateImage(allocator, &image_info, &allocation_info, &image_, &allocation, NULL);
  allocation_ = allocation;

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.image = image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format_;
  if (is_depth) {
    view_info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
  } else {
    view_info.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                            VK_COMPONENT_SWIZZLE_A};
    view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  }
  vkCreateImageView(*device_, &view_info, nullptr, &image_view_);
}

ImageImpl::~ImageImpl() {
  VmaAllocator allocator = static_cast<VmaAllocator>(device_->allocator());
  VmaAllocation allocation = static_cast<VmaAllocation>(allocation_);

  vkDestroyImageView(*device_, image_view_, nullptr);
  vmaDestroyImage(allocator, image_, allocation);
}

template class SharedAccessor<Image, ImageImpl>;

}  // namespace gpu
}  // namespace vkgs
