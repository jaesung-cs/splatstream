#include "imgui_texture.h"

#include "imgui_impl_vulkan.h"

#include "vkgs/gpu/object.h"

namespace vkgs {
namespace viewer {

class ImGuiTextureImpl : public gpu::Object {
 public:
  void __init__(VkSampler sampler, VkImageView image_view) {
    texture_ = ImGui_ImplVulkan_AddTexture(sampler, image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  void __del__() { ImGui_ImplVulkan_RemoveTexture(texture_); }

  operator VkDescriptorSet() const noexcept { return texture_; }

 private:
  VkDescriptorSet texture_ = VK_NULL_HANDLE;
};

ImGuiTexture ImGuiTexture::Create(VkSampler sampler, VkImageView image_view) {
  return Make<ImGuiTextureImpl>(sampler, image_view);
}

ImGuiTexture::operator VkDescriptorSet() const { return *impl_; }

}  // namespace viewer
}  // namespace vkgs
