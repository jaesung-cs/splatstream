#ifndef VKGS_GPU_IMAGE_H
#define VKGS_GPU_IMAGE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class ImageImpl;
class VKGS_GPU_API Image : public Handle<Image, ImageImpl> {
 public:
  static Image Create(VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage);

  operator VkImage() const;
  VkImageView image_view() const;
  VkFormat format() const;
  uint32_t width() const;
  uint32_t height() const;

  void Keep() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_IMAGE_H
