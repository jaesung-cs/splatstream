#include "imgui_texture.h"

#include "imgui_impl_vulkan.h"

namespace vkgs {
namespace viewer {

std::shared_ptr<ImGuiTexture> ImGuiTexture::Create(VkSampler sampler, VkImageView image_view) {
  return std::make_shared<ImGuiTexture>(sampler, image_view);
}

ImGuiTexture::ImGuiTexture(VkSampler sampler, VkImageView image_view) {
  texture_ = ImGui_ImplVulkan_AddTexture(sampler, image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

ImGuiTexture::~ImGuiTexture() { ImGui_ImplVulkan_RemoveTexture(texture_); }

}  // namespace viewer
}  // namespace vkgs
