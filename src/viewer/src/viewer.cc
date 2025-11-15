#include "vkgs/viewer/viewer.h"

#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/swapchain.h"
#include "vkgs/gpu/queue.h"

namespace vkgs {
namespace viewer {

void Init() {
  glfwInit();

  // GLFW required instance extensions
  uint32_t count;
  const char** instance_extensions = glfwGetRequiredInstanceExtensions(&count);

  gpu::DeviceCreateInfo device_info;
  device_info.instance_extensions.assign(instance_extensions, instance_extensions + count);
  gpu::Init(device_info);
}

void Terminate() {
  gpu::Terminate();
  glfwTerminate();
}

Viewer::Viewer() = default;

Viewer::~Viewer() = default;

void Viewer::Run() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  ImGui::StyleColorsDark();

  auto device = gpu::GetDevice();
  auto instance = device->instance();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(1280, 720, "vkgs", nullptr, nullptr);
  if (!window_) throw std::runtime_error("Failed to create window");

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  glfwCreateWindowSurface(instance, window_, NULL, &surface);
  {
    gpu::Swapchain swapchain(device, surface);

    VkFormat format = swapchain.format();

    ImGui_ImplGlfw_InitForVulkan(window_, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = device->physical_device();
    init_info.Device = *device;
    init_info.QueueFamily = device->graphics_queue_index();
    init_info.Queue = *device->graphics_queue();
    init_info.DescriptorPoolSize = 1024;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
    init_info.UseDynamicRendering = true;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info);

    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();

      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("Hello, world!");
      ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
      ImGui::End();

      ImGui::Render();
      ImDrawData* draw_data = ImGui::GetDrawData();

      const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
      if (!is_minimized) {
        auto present_image_info = swapchain.AcquireNextImage();

        device
            ->GraphicsTask([=](VkCommandBuffer cb) {
              gpu::cmd::Barrier()
                  .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, present_image_info.image)
                  .Commit(cb);

              // Rendering
              VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
              color_attachment.imageView = present_image_info.image_view;
              color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
              color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
              color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
              color_attachment.clearValue.color = {0.f, 0.f, 0.f, 1.f};
              VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
              rendering_info.renderArea.offset = {0, 0};
              rendering_info.renderArea.extent = present_image_info.extent;
              rendering_info.layerCount = 1;
              rendering_info.colorAttachmentCount = 1;
              rendering_info.pColorAttachments = &color_attachment;
              vkCmdBeginRendering(cb, &rendering_info);

              ImGui_ImplVulkan_RenderDrawData(draw_data, cb);

              vkCmdEndRendering(cb);

              gpu::cmd::Barrier()
                  .Image(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0, 0,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                         present_image_info.image)
                  .Commit(cb);
            })
            .Wait(present_image_info.image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
            .Signal(present_image_info.render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
            .Submit();

        swapchain.Present();
      }
    }

    device->WaitIdle();
  }

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
}

}  // namespace viewer
}  // namespace vkgs
