#include "imgui_texture.h"

#include "imgui_impl_vulkan.h"

namespace vkgs {
namespace viewer {

ImGuiTextureImpl::ImGuiTextureImpl(VkSampler sampler, VkImageView image_view) {
  texture_ = ImGui_ImplVulkan_AddTexture(sampler, image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

ImGuiTextureImpl::~ImGuiTextureImpl() { ImGui_ImplVulkan_RemoveTexture(texture_); }

}  // namespace viewer
}  // namespace vkgs
