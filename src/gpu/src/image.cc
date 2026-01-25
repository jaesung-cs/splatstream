#include "vkgs/gpu/image.h"

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class ImageImpl : public Object {
 public:
  // TODO: red-alpha -> generalize swizzle
  void __init__(VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage) {
    format_ = format;
    width_ = width;
    height_ = height;

    bool is_depth = false;
    switch (format) {
      case VK_FORMAT_D16_UNORM:
      case VK_FORMAT_D24_UNORM_S8_UINT:
      case VK_FORMAT_D32_SFLOAT:
        is_depth = true;
        break;
    }

    VmaAllocator allocator = static_cast<VmaAllocator>(device_.allocator());
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
    vkCreateImageView(device_, &view_info, nullptr, &image_view_);
  }

  void __del__() {
    vkDestroyImageView(device_, image_view_, nullptr);
    vmaDestroyImage(device_.allocator(), image_, allocation_);
  }

  operator VkImage() const noexcept { return image_; }

  VkImageView image_view() const noexcept { return image_view_; }
  VkFormat format() const noexcept { return format_; }
  uint32_t width() const noexcept { return width_; }
  uint32_t height() const noexcept { return height_; }

 private:
  VkImage image_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  VkImageView image_view_ = VK_NULL_HANDLE;
  VkFormat format_;
  uint32_t width_;
  uint32_t height_;
};

Image Image::Create(VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage) {
  return Make<ImageImpl>(format, width, height, usage);
}

Image::operator VkImage() const { return *impl_; }
VkImageView Image::image_view() const { return impl_->image_view(); }
VkFormat Image::format() const { return impl_->format(); }
uint32_t Image::width() const { return impl_->width(); }
uint32_t Image::height() const { return impl_->height(); }

void Image::Keep() const { impl_->Keep(); }

}  // namespace gpu
}  // namespace vkgs
