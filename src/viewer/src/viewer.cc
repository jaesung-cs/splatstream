#include "vkgs/viewer/viewer.h"

#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/swapchain.h"
#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/task.h"
#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/cmd/pipeline.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/graphics_pipeline.h"

#include "vkgs/core/parser.h"
#include "vkgs/core/renderer.h"
#include "vkgs/core/screen_splats.h"
#include "vkgs/core/gaussian_splats.h"

#include "generated/blend_vert.h"
#include "generated/blend_frag.h"
#include "camera.h"
#include "context.h"

namespace vkgs {
namespace viewer {

Viewer::Viewer() { context_ = GetContext(); }

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

  auto swapchain = std::make_shared<gpu::Swapchain>(surface);

  VkFormat swapchain_format = swapchain->format();
  VkFormat low_format = VK_FORMAT_B8G8R8A8_UNORM;
  VkFormat high_format = VK_FORMAT_R16G16B16A16_SFLOAT;
  std::vector<VkFormat> formats = {swapchain_format, low_format, high_format};

  // Blend pipeline
  auto graphics_pipeline_layout =
      gpu::PipelineLayout::Create({{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                   {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT}});

  gpu::GraphicsPipelineCreateInfo blend_pipeline_info = {};
  blend_pipeline_info.pipeline_layout = *graphics_pipeline_layout;
  blend_pipeline_info.vertex_shader = gpu::ShaderCode(blend_vert);
  blend_pipeline_info.fragment_shader = gpu::ShaderCode(blend_frag);
  blend_pipeline_info.formats = formats;
  blend_pipeline_info.locations = {0, VK_ATTACHMENT_UNUSED, VK_ATTACHMENT_UNUSED};
  blend_pipeline_info.input_indices = {VK_ATTACHMENT_UNUSED, 0, 1};
  auto blend_pipeline = gpu::GraphicsPipeline::Create(blend_pipeline_info);

  auto cq = device->compute_queue();
  auto gq = device->graphics_queue();

  // ImGui rendering
  ImGui_ImplGlfw_InitForVulkan(window_, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance;
  init_info.PhysicalDevice = device->physical_device();
  init_info.Device = *device;
  init_info.QueueFamily = *gq;
  init_info.Queue = *gq;
  init_info.DescriptorPoolSize = 1024;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_format;
  init_info.UseDynamicRendering = true;
  init_info.CheckVkResultFn = nullptr;
  ImGui_ImplVulkan_Init(&init_info);

  // TODO: load model
  auto parser = std::make_shared<core::Parser>();
  auto renderer = std::make_shared<core::Renderer>();
  auto splats = parser->LoadFromPly("./test_random/gsplat.ply");

  auto camera = std::make_shared<Camera>();

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    const auto& io = ImGui::GetIO();
    bool is_minimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;

    // handle events
    if (!io.WantCaptureMouse) {
      bool left = io.MouseDown[ImGuiMouseButton_Left];
      bool right = io.MouseDown[ImGuiMouseButton_Right];
      bool ctrl = ImGui::IsKeyDown(ImGuiKey::ImGuiMod_Ctrl);
      float dx = io.MouseDelta.x;
      float dy = io.MouseDelta.y;

      if (left && !right) {
        // ctrl + drag for translation, otherwise rotate
        if (ctrl) {
          camera->Translate(dx, dy);
        } else {
          camera->Rotate(dx, dy);
        }
      } else if (!left && right) {
        camera->Translate(dx, dy);
      } else if (left && right) {
        camera->Zoom(dy);
      }

      if (io.MouseWheel != 0.f) {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
          camera->DollyZoom(io.MouseWheel);
        } else {
          camera->Zoom(io.MouseWheel * 10.f);
        }
      }
    }

    if (!io.WantCaptureKeyboard) {
      constexpr float speed = 1000.f;
      float dt = io.DeltaTime;
      if (ImGui::IsKeyDown(ImGuiKey_W)) {
        camera->Translate(0.f, 0.f, speed * dt);
      }
      if (ImGui::IsKeyDown(ImGuiKey_S)) {
        camera->Translate(0.f, 0.f, -speed * dt);
      }
      if (ImGui::IsKeyDown(ImGuiKey_A)) {
        camera->Translate(speed * dt, 0.f);
      }
      if (ImGui::IsKeyDown(ImGuiKey_D)) {
        camera->Translate(-speed * dt, 0.f);
      }
      if (ImGui::IsKeyDown(ImGuiKey_Space)) {
        camera->Translate(0.f, speed * dt);
      }
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGui::Begin("Hello, world!");
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
    ImGui::End();

    // Gizmo
    glm::mat4 y_flip(1.f);
    y_flip[1][1] = -1.f;
    auto view = camera->ViewMatrix();
    auto projection = y_flip * camera->ProjectionMatrix();  // In ImGuizmo's OpenGL coordinate system
    static glm::mat4 model(1.f);
    if (!is_minimized) {
      ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    }
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), ImGuizmo::UNIVERSAL, ImGuizmo::LOCAL,
                         glm::value_ptr(model));

    if (is_minimized) {
      ImGui::EndFrame();
    } else {
      ImGui::Render();
      ImDrawData* draw_data = ImGui::GetDrawData();

      auto present_image_info = swapchain->AcquireNextImage();
      auto width = present_image_info.extent.width;
      auto height = present_image_info.extent.height;

      camera->SetWindowSize(width, height);

      core::DrawOptions draw_options = {};
      draw_options.view = camera->ViewMatrix();
      draw_options.projection = camera->ProjectionMatrix();
      draw_options.model = model;
      draw_options.width = width;
      draw_options.height = height;
      draw_options.background = {0.f, 0.f, 0.f};
      draw_options.eps2d = 0.01f;
      draw_options.sh_degree = splats->sh_degree();

      // TODO: ring buffer
      auto screen_splats = std::make_shared<core::ScreenSplats>();
      screen_splats->Update(splats->size());
      auto csem = device->AllocateSemaphore();
      auto cval = csem->value();
      auto gsem = device->AllocateSemaphore();
      auto gval = gsem->value();

      auto image8 = gpu::Image::Create(low_format, width, height,
                                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                           VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
      auto image16 = gpu::Image::Create(high_format, width, height,
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

      // Compute queue
      {
        gpu::ComputeTask task;
        auto cb = task.command_buffer();

        // Compute
        renderer->ComputeScreenSplats(cb, splats, draw_options, screen_splats);

        // Release
        gpu::cmd::Barrier()
            .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, *cq, *gq,
                     *screen_splats->instances())
            .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, *cq, *gq,
                     *screen_splats->draw_indirect())
            .Commit(cb);

        task.Signal(*csem, cval + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
      }

      // Graphics queue
      {
        gpu::GraphicsTask task;
        auto cb = task.command_buffer();

        gpu::cmd::Barrier()
            // Acquire
            .Acquire(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *cq, *gq,
                     *screen_splats->instances())
            .Acquire(VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, *cq, *gq,
                     *screen_splats->draw_indirect())
            // Image layout transition
            .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, present_image_info.image)
            .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, *image8)
            .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, *image16)
            .Commit(cb);

        // Rendering
        std::array<VkRenderingAttachmentInfo, 3> color_attachments;

        // Present image
        color_attachments[0] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachments[0].imageView = present_image_info.image_view;
        color_attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments[0].clearValue.color = {0.f, 0.f, 0.f, 0.f};
        // Scene image
        color_attachments[1] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachments[1].imageView = image8->image_view();
        color_attachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachments[1].clearValue.color = {0.5f, 0.5f, 0.5f, 1.f};  // TODO: background color
        // Splats image
        color_attachments[2] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachments[2].imageView = image16->image_view();
        color_attachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachments[2].clearValue.color = {0.f, 0.f, 0.f, 0.f};
        VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        rendering_info.renderArea.offset = {0, 0};
        rendering_info.renderArea.extent = present_image_info.extent;
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = color_attachments.size();
        rendering_info.pColorAttachments = color_attachments.data();
        vkCmdBeginRendering(cb, &rendering_info);

        VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
        vkCmdSetViewport(cb, 0, 1, &viewport);
        VkRect2D scissor = {0, 0, width, height};
        vkCmdSetScissor(cb, 0, 1, &scissor);

        // subpass 0: render scene
        // TODO

        // subpass 1: render splats
        // TODO: wait for depth attachment
        {
          // location 0: image16
          VkRenderingAttachmentLocationInfo location_info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_LOCATION_INFO};
          std::vector<uint32_t> locations = {VK_ATTACHMENT_UNUSED, VK_ATTACHMENT_UNUSED, 0};
          location_info.colorAttachmentCount = locations.size();
          location_info.pColorAttachmentLocations = locations.data();
          vkCmdSetRenderingAttachmentLocations(cb, &location_info);
          renderer->RenderScreenSplats(cb, splats, draw_options, screen_splats, formats, locations);
        }

        // subpass 2: blend
        gpu::cmd::Barrier(VK_DEPENDENCY_BY_REGION_BIT)
            .Memory(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT)
            .Commit(cb);

        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
            .Input(0, image8->image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
            .Input(1, image16->image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
            .Bind(*blend_pipeline)
            .Commit(cb);

        {
          VkRenderingAttachmentLocationInfo location_info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_LOCATION_INFO};
          std::array<uint32_t, 3> locations = {0, VK_ATTACHMENT_UNUSED, VK_ATTACHMENT_UNUSED};
          location_info.colorAttachmentCount = locations.size();
          location_info.pColorAttachmentLocations = locations.data();
          vkCmdSetRenderingAttachmentLocations(cb, &location_info);

          VkRenderingInputAttachmentIndexInfo input_attachment_index_info = {
              VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO};
          std::array<uint32_t, 3> input_indices = {VK_ATTACHMENT_UNUSED, 0, 1};
          input_attachment_index_info.colorAttachmentCount = input_indices.size();
          input_attachment_index_info.pColorAttachmentInputIndices = input_indices.data();
          vkCmdSetRenderingInputAttachmentIndices(cb, &input_attachment_index_info);
        }

        vkCmdDraw(cb, 3, 1, 0, 0);

        vkCmdEndRendering(cb);

        // Wait for color attachment write to complete, before rendering UI.
        gpu::cmd::Barrier()
            .Memory(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
            .Commit(cb);

        // Rendering
        VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachment.imageView = present_image_info.image_view;
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
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
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, present_image_info.image)
            .Commit(cb);

        image8->Keep();
        image16->Keep();

        task.Wait(present_image_info.image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        task.Wait(*csem, cval + 1,
                  VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
        task.Signal(*gsem, gval + 1, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        task.Signal(present_image_info.render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
      }

      csem->Increment();
      gsem->Increment();

      swapchain->Present();
    }
  }

  device->WaitIdle();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  swapchain = nullptr;
  cq = nullptr;
  gq = nullptr;
  graphics_pipeline_layout = nullptr;
  blend_pipeline = nullptr;
  parser = nullptr;
  renderer = nullptr;
  splats = nullptr;
  device = nullptr;

  glfwDestroyWindow(window_);
}

}  // namespace viewer
}  // namespace vkgs
