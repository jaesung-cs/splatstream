#include "viewer_base.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include "implot.h"

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/queue.h"

#include "fonts/roboto_regular.h"

namespace vkgs {
namespace viewer {

void ViewerBase::__init__() { context_ = GetContext(); }

void ViewerBase::__del__() {}

void ViewerBase::Run() {
  InitializeWindow();
  OnBeforeRun();

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    DrawUi();

    const auto& io = ImGui::GetIO();
    bool is_minimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;

    if (is_minimized) {
      ImGui::EndFrame();
    } else {
      ImGui::Render();
      auto present_image_info = swapchain_.AcquireNextImage();
      Draw(present_image_info);
      swapchain_.Present();
    }
  }

  OnAfterRun();
  FinalizeWindow();
}

void ViewerBase::DrawUi() {}

void ViewerBase::Draw(const gpu::PresentImageInfo& present) {}

void ViewerBase::InitializeWindow() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(1600, 900, "vkgs", nullptr, nullptr);
  if (!window_) throw std::runtime_error("Failed to create window");

  auto device = context_.device();
  auto instance = device.instance();
  glfwCreateWindowSurface(instance, window_, NULL, &surface_);

  swapchain_ = gpu::Swapchain::Create(surface_, swapchain_format_,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  auto gq = device.graphics_queue();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImPlot::CreateContext();

  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.IniFilename = NULL;
  io.LogFilename = NULL;

  ImFontConfig font_config = {};
  font_config.OversampleH = 3;
  font_config.OversampleV = 3;
  float font_size = 15.f;
  io.Fonts->AddFontFromMemoryCompressedTTF(roboto_regular_compressed_data, roboto_regular_compressed_size, font_size,
                                           &font_config);
  auto& style = ImGui::GetStyle();
  style.FramePadding.y = 2;
  style.ItemSpacing.y = 3;

  ImGui_ImplGlfw_InitForVulkan(window_, true);
  ImGui_ImplVulkan_InitInfo init_info = {
      .Instance = device.instance(),
      .PhysicalDevice = device,
      .Device = device,
      .QueueFamily = gq,
      .Queue = gq,
      .DescriptorPoolSize = 1024,
      .MinImageCount = 3,
      .ImageCount = 3,
      .PipelineInfoMain =
          {
              .PipelineRenderingCreateInfo =
                  {
                      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                      .colorAttachmentCount = 1,
                      .pColorAttachmentFormats = &swapchain_format_,
                  },
          },
      .UseDynamicRendering = true,
      .CheckVkResultFn = nullptr,
  };
  ImGui_ImplVulkan_Init(&init_info);
}

void ViewerBase::FinalizeWindow() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
}

gpu::Device ViewerBase::device() const { return context_.device(); }

}  // namespace viewer
}  // namespace vkgs
