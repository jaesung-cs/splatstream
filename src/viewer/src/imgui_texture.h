#ifndef VKGS_VIEWER_IMGUI_TEXTURE_H
#define VKGS_VIEWER_IMGUI_TEXTURE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace viewer {

class ImGuiTextureImpl : public gpu::Object {
 public:
  ImGuiTextureImpl(VkSampler sampler, VkImageView image_view);
  ~ImGuiTextureImpl() override;

  operator VkDescriptorSet() const noexcept { return texture_; }

 private:
  VkDescriptorSet texture_ = VK_NULL_HANDLE;
};

class ImGuiTexture : public SharedAccessor<ImGuiTexture, ImGuiTextureImpl> {};

}  // namespace viewer
}  // namespace vkgs

#endif