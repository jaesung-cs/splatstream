#ifndef VKGS_VIEWER_IMGUI_TEXTURE_H
#define VKGS_VIEWER_IMGUI_TEXTURE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace viewer {

class ImGuiTextureImpl;
class ImGuiTexture : public Handle<ImGuiTexture, ImGuiTextureImpl> {
 public:
  static ImGuiTexture Create(VkSampler sampler, VkImageView image_view);

  operator VkDescriptorSet() const;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_IMGUI_TEXTURE_H
