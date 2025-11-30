#ifndef VKGS_VIEWER_IMGUI_TEXTURE_H
#define VKGS_VIEWER_IMGUI_TEXTURE_H

#include <vulkan/vulkan.h>

#include "vkgs/gpu/object.h"

namespace vkgs {
namespace viewer {

class ImGuiTexture : public gpu::Object {
 public:
  static std::shared_ptr<ImGuiTexture> Create(VkSampler sampler, VkImageView image_view);

 public:
  ImGuiTexture(VkSampler sampler, VkImageView image_view);
  ~ImGuiTexture() override;

  operator VkDescriptorSet() const noexcept { return texture_; }

 private:
  VkDescriptorSet texture_ = VK_NULL_HANDLE;
};

}  // namespace viewer
}  // namespace vkgs

#endif