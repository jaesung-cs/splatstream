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

#include "generated/color_vert.h"
#include "generated/color_frag.h"
#include "generated/blend_vert.h"
#include "generated/blend_frag.h"
#include "camera.h"
#include "context.h"

namespace vkgs {
namespace viewer {

Viewer::Viewer() { context_ = GetContext(); }

Viewer::~Viewer() = default;

void Viewer::Run() {
  if (model_path_.empty()) throw std::runtime_error("Model path is not set");

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

  auto swapchain = std::make_shared<gpu::Swapchain>(
      surface, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  VkFormat swapchain_format = swapchain->format();
  VkFormat high_format = VK_FORMAT_R16G16B16A16_SFLOAT;
  VkFormat depth_image_format = VK_FORMAT_R16G16_SFLOAT;
  VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
  std::vector<VkFormat> formats = {swapchain_format, high_format, depth_image_format};

  // Graphics pipelines
  struct ColorPushConstants {
    glm::mat4 projection;
    glm::mat4 view;
  };

  struct BlendPushConstants {
    int mode;
  };

  auto graphics_pipeline_layout = gpu::PipelineLayout::Create(
      {{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
       {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT}},
      {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants)},
       {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ColorPushConstants), sizeof(BlendPushConstants)}});

  gpu::GraphicsPipelineCreateInfo color_pipeline_info = {};
  color_pipeline_info.pipeline_layout = *graphics_pipeline_layout;
  color_pipeline_info.vertex_shader = gpu::ShaderCode(color_vert);
  color_pipeline_info.fragment_shader = gpu::ShaderCode(color_frag);
  color_pipeline_info.formats = formats;
  color_pipeline_info.locations = {VK_ATTACHMENT_UNUSED, 0, 1};
  color_pipeline_info.depth_format = depth_format;
  color_pipeline_info.depth_test = true;
  color_pipeline_info.depth_write = true;
  auto color_pipeline = gpu::GraphicsPipeline::Create(color_pipeline_info);

  gpu::GraphicsPipelineCreateInfo blend_pipeline_info = {};
  blend_pipeline_info.pipeline_layout = *graphics_pipeline_layout;
  blend_pipeline_info.vertex_shader = gpu::ShaderCode(blend_vert);
  blend_pipeline_info.fragment_shader = gpu::ShaderCode(blend_frag);
  blend_pipeline_info.formats = formats;
  blend_pipeline_info.locations = {0, VK_ATTACHMENT_UNUSED, VK_ATTACHMENT_UNUSED};
  blend_pipeline_info.input_indices = {VK_ATTACHMENT_UNUSED, 0, 1};
  blend_pipeline_info.depth_format = depth_format;
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

  // Load model
  auto parser = std::make_shared<core::Parser>();
  auto renderer = std::make_shared<core::Renderer>();
  auto splats = parser->LoadFromPly(model_path_);

  auto camera = std::make_shared<Camera>();

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    const auto& io = ImGui::GetIO();
    bool is_minimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;

    // handle events
    if (!io.WantCaptureMouse || (ImGuizmo::IsOver() && !ImGuizmo::IsUsing() && !ImGui::IsAnyItemActive())) {
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

    static glm::mat4 model(1.f);
    static bool vsync = true;
    static int sh_degree = splats->sh_degree();
    static int render_type = 0;
    static glm::vec3 background(0.f, 0.f, 0.f);

    // Gizmo
    glm::mat4 vk_to_gl(1.f);
    // y-axis flip
    vk_to_gl[1][1] = -1.f;
    // z: (0, 1) -> (-1, 1)
    vk_to_gl[2][2] = 2.f;
    vk_to_gl[3][2] = -1.f;
    auto view = camera->ViewMatrix();
    auto projection = vk_to_gl * camera->ProjectionMatrix();
    if (!is_minimized) ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                         ImGuizmo::TRANSLATE | ImGuizmo::ROTATE | ImGuizmo::SCALE_X, ImGuizmo::LOCAL,
                         glm::value_ptr(model));

    if (ImGui::Begin("vkgs viewer")) {
      ImGui::Text("FPS: %.2f", io.Framerate);

      if (ImGui::Checkbox("vsync", &vsync)) {
        if (vsync)
          swapchain->SetPresentMode(VK_PRESENT_MODE_FIFO_KHR);
        else
          swapchain->SetPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
      }

      ImGui::SliderInt("SH degree", &sh_degree, 0, splats->sh_degree());

      constexpr const char* render_types[] = {"Color", "Alpha", "Depth"};
      ImGui::Combo("Render type", &render_type, render_types, IM_ARRAYSIZE(render_types));

      ImGui::ColorEdit3("Background", glm::value_ptr(background));

      // Apply X scale to Y and Z
      glm::vec3 translation;
      glm::vec3 rotation;
      glm::vec3 scale;
      ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation),
                                            glm::value_ptr(rotation), glm::value_ptr(scale));

      ImGui::DragFloat3("Translation", glm::value_ptr(translation), 0.01f);
      ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f);
      ImGui::SliderFloat("Scale", &scale.x, 0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
      scale.y = scale.z = scale.x;
      ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(translation), glm::value_ptr(rotation),
                                              glm::value_ptr(scale), glm::value_ptr(model));
    }
    ImGui::End();

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
      draw_options.background = {0.f, 0.f, 0.f};  // unused
      draw_options.eps2d = 0.01f;
      draw_options.sh_degree = render_type == 0 ? sh_degree : 0;

      // ring buffer
      auto& storage = ring_buffer_[frame_index_ % ring_buffer_.size()];
      storage.Update(splats->size(), width, height);
      auto screen_splats = storage.screen_splats();
      auto csem = storage.compute_semaphore();
      auto cval = csem->value();
      auto gsem = storage.graphics_semaphore();
      auto gval = gsem->value();
      auto image16 = storage.image();
      auto depth = storage.depth();
      auto depth_image = storage.depth_image();
      frame_index_++;

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
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, *image16)
            .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, *depth_image)
            .Image(0, 0, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                   VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, *depth)
            .Commit(cb);

        // Rendering
        std::array<VkRenderingAttachmentInfo, 3> color_attachments;
        // Swapchain image
        color_attachments[0] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachments[0].imageView = present_image_info.image_view;
        color_attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // TODO: don't clear, no blend
        color_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments[0].clearValue.color = {0.f, 0.f, 0.f, 1.f};
        // Splats image
        color_attachments[1] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachments[1].imageView = image16->image_view();
        color_attachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachments[1].clearValue.color = {background.r, background.g, background.b, 0.f};
        // Depth image
        color_attachments[2] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachments[2].imageView = depth_image->image_view();
        color_attachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachments[2].clearValue.color = {0.f, 0.f, 0.f, 0.f};

        VkRenderingAttachmentInfo depth_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depth_attachment.imageView = depth->image_view();
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.clearValue.depthStencil = {1.f, 0};

        VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        rendering_info.renderArea.offset = {0, 0};
        rendering_info.renderArea.extent = present_image_info.extent;
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = color_attachments.size();
        rendering_info.pColorAttachments = color_attachments.data();
        rendering_info.pDepthAttachment = &depth_attachment;
        vkCmdBeginRendering(cb, &rendering_info);

        VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
        vkCmdSetViewport(cb, 0, 1, &viewport);
        VkRect2D scissor = {0, 0, width, height};
        vkCmdSetScissor(cb, 0, 1, &scissor);

        // render scene
        // TODO: draw scene to color/depth image
        if (false) {
          ColorPushConstants color_push_constants = {};
          color_push_constants.projection = camera->ProjectionMatrix();
          color_push_constants.view = camera->ViewMatrix();
          gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
              .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants), &color_push_constants)
              .Bind(*color_pipeline)
              .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0, 1})
              .Commit(cb);

          vkCmdDraw(cb, 6, 1, 0, 0);
        }

        // render splats
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
            .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0, render_type == 2 ? 1 : VK_ATTACHMENT_UNUSED})
            .Commit(cb);
        renderer->RenderScreenSplats(cb, splats, draw_options, screen_splats, formats,
                                     {VK_ATTACHMENT_UNUSED, 0, render_type == 2 ? 1 : VK_ATTACHMENT_UNUSED},
                                     depth_format);

        // subpass 1:
        gpu::cmd::Barrier(VK_DEPENDENCY_BY_REGION_BIT)
            .Memory(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT)
            .Commit(cb);

        BlendPushConstants blend_push_constants = {};
        blend_push_constants.mode = render_type;
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
            .Input(0, image16->image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
            .Input(1, depth_image->image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
            .PushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ColorPushConstants), sizeof(BlendPushConstants),
                          &blend_push_constants)
            .Bind(*blend_pipeline)
            .AttachmentLocations({0, VK_ATTACHMENT_UNUSED, VK_ATTACHMENT_UNUSED})
            .InputAttachmentIndices({VK_ATTACHMENT_UNUSED, 0, 1})
            .Commit(cb);

        vkCmdDraw(cb, 3, 1, 0, 0);

        vkCmdEndRendering(cb);

        // Rendering UI
        gpu::cmd::Barrier()
            .Memory(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
            .Commit(cb);

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

        image16->Keep();
        depth_image->Keep();
        depth->Keep();

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
  color_pipeline = nullptr;
  blend_pipeline = nullptr;
  parser = nullptr;
  renderer = nullptr;
  splats = nullptr;
  device = nullptr;

  glfwDestroyWindow(window_);
}

}  // namespace viewer
}  // namespace vkgs
