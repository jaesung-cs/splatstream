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
#include "vkgs/gpu/semaphore.h"
#include "vkgs/core/renderer.h"
#include "vkgs/core/compute_storage.h"
#include "vkgs/core/gaussian_splats.h"

namespace vkgs {
namespace viewer {

void Init() {
  glfwInit();

  // GLFW required instance extensions
  uint32_t count;
  const char** instance_extensions = glfwGetRequiredInstanceExtensions(&count);

  gpu::DeviceCreateInfo device_info;
  device_info.enable_viewer = true;
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

  auto swapchain = std::make_shared<gpu::Swapchain>(device, surface);
  VkFormat format = swapchain->format();

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

  // TODO: load model
  auto renderer = std::make_shared<core::Renderer>();
  auto splats = renderer->LoadFromPly("./test_random/gsplat.ply");

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
      auto present_image_info = swapchain->AcquireNextImage();

      // TODO: camera
      core::DrawOptions draw_options = {};
      draw_options.view = glm::mat4(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, -10, 1);
      draw_options.projection =
          glm::mat4(0.57735027, 0, 0, 0, 0, -0.57735027, 0, 0, 0, 0, -1.0000001, -1, 0, 0, -0.01, 0);
      draw_options.width = present_image_info.extent.width;
      draw_options.height = present_image_info.extent.height;
      draw_options.background = {0.f, 0.f, 0.f};
      draw_options.eps2d = 0.01f;
      draw_options.sh_degree = splats->sh_degree();

      // TODO: ring buffer
      auto compute_storage = std::make_shared<core::ComputeStorage>(device);
      renderer->UpdateComputeStorage(compute_storage, splats->size());
      auto csem = device->AllocateSemaphore();
      auto cval = csem->value();
      auto gsem = device->AllocateSemaphore();
      auto gval = gsem->value();

      device
          ->ComputeTask(
              [=](VkCommandBuffer cb) { renderer->ComputeScreenSplats(cb, splats, draw_options, compute_storage); })
          .Signal(*csem, cval + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
          .Submit();

      device
          ->GraphicsTask([=](VkCommandBuffer cb) {
            renderer->AcquireScreenSplats(cb, compute_storage);

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
            // TODO: background color
            color_attachment.clearValue.color = {0.f, 0.f, 0.f, 0.f};
            VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
            rendering_info.renderArea.offset = {0, 0};
            rendering_info.renderArea.extent = present_image_info.extent;
            rendering_info.layerCount = 1;
            rendering_info.colorAttachmentCount = 1;
            rendering_info.pColorAttachments = &color_attachment;
            vkCmdBeginRendering(cb, &rendering_info);

            renderer->RenderScreenSplats(cb, splats, draw_options, compute_storage);

            ImGui_ImplVulkan_RenderDrawData(draw_data, cb);

            vkCmdEndRendering(cb);

            gpu::cmd::Barrier()
                .Image(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0, 0,
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                       present_image_info.image)
                .Commit(cb);
          })
          .Wait(present_image_info.image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
          .Wait(*csem, cval + 1,
                VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)
          .Signal(*gsem, gval + 1, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
          .Signal(present_image_info.render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
          .Submit();

      csem->Increment();
      gsem->Increment();

      swapchain->Present();
    }
  }

  swapchain = nullptr;

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
}

}  // namespace viewer
}  // namespace vkgs
