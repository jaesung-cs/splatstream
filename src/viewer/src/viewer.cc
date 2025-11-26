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
namespace {

glm::mat4 OpenCVExtrinsicToView(const glm::mat4& extrinsic) {
  // OpenCV convention: x right, y down, z forward.
  // To glm convention: x right, y up, z backward.
  glm::mat4 view = glm::mat4(1.f);
  view[1][1] = -1.f;
  view[2][2] = -1.f;
  return view * extrinsic;
}

}  // namespace

Viewer::Viewer() { context_ = GetContext(); }

Viewer::~Viewer() = default;

void Viewer::Run() {
  bool own_renderer = false;
  if (!renderer_) {
    renderer_ = std::make_shared<core::Renderer>();
    own_renderer = true;
  }

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
    glm::mat4 model;
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
  color_pipeline_info.bindings = {
      {0, sizeof(float) * 6, VK_VERTEX_INPUT_RATE_VERTEX},
  };
  color_pipeline_info.attributes = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 0},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3},
  };
  color_pipeline_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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

  auto camera = std::make_shared<Camera>();

  // Initialize camera with first camera params
  if (!camera_params_.empty()) {
    const auto& camera_params = camera_params_[0];
    camera->SetView(OpenCVExtrinsicToView(camera_params.extrinsic));
    camera->Update(10.f);
  }

  // Camera
  std::shared_ptr<gpu::Buffer> camera_vertices;
  std::shared_ptr<gpu::Buffer> camera_indices;
  uint32_t camera_index_size = 0;
  {
    gpu::GraphicsTask task;
    auto cb = task.command_buffer();

    std::vector<uint32_t> indices = {
        0, 1, 0, 2, 0, 3, 0, 4,  // legs
        1, 2, 2, 4, 4, 3, 3, 1,  // frames
    };
    std::vector<float> vertices = {
        0.f,  0.f,  0.f, 1.f, 1.f, 1.f,  // eye
        -1.f, -1.f, 1.f, 1.f, 1.f, 1.f,  // 0
        1.f,  -1.f, 1.f, 1.f, 1.f, 1.f,  // 1
        -1.f, 1.f,  1.f, 1.f, 1.f, 1.f,  // 2
        1.f,  1.f,  1.f, 1.f, 1.f, 1.f,  // 3
    };

    camera_index_size = indices.size();

    auto indices_stage =
        gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indices.size() * sizeof(indices[0]), true);
    auto vertices_stage =
        gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertices.size() * sizeof(vertices[0]), true);

    std::memcpy(indices_stage->data(), indices.data(), indices.size() * sizeof(indices[0]));
    std::memcpy(vertices_stage->data(), vertices.data(), vertices.size() * sizeof(vertices[0]));

    camera_vertices = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                          vertices.size() * sizeof(vertices[0]));
    camera_indices = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                         indices.size() * sizeof(indices[0]));

    VkBufferCopy region = {0, 0, indices_stage->size()};
    vkCmdCopyBuffer(cb, *indices_stage, *camera_indices, 1, &region);
    region = {0, 0, vertices_stage->size()};
    vkCmdCopyBuffer(cb, *vertices_stage, *camera_vertices, 1, &region);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT)
        .Commit(cb);

    indices_stage->Keep();
    vertices_stage->Keep();
    camera_vertices->Keep();
    camera_indices->Keep();

    task.Submit();
  }

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    const auto& io = ImGui::GetIO();
    bool is_minimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;
    bool camera_modified = false;

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
          if (dx != 0.f || dy != 0.f) camera_modified = true;
        } else {
          camera->Rotate(dx, dy);
          if (dx != 0.f || dy != 0.f) camera_modified = true;
        }
      } else if (!left && right) {
        camera->Translate(dx, dy);
        if (dx != 0.f || dy != 0.f) camera_modified = true;
      } else if (left && right) {
        camera->Zoom(dy);
        if (dy != 0.f) camera_modified = true;
      }

      if (io.MouseWheel != 0.f) {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
          camera->DollyZoom(io.MouseWheel);
          camera_modified = true;
        } else {
          camera->Zoom(io.MouseWheel * 10.f);
          camera_modified = true;
        }
      }
    }

    if (!io.WantCaptureKeyboard) {
      constexpr float speed = 1000.f;
      float dt = io.DeltaTime;
      if (ImGui::IsKeyDown(ImGuiKey_W)) {
        camera->Translate(0.f, 0.f, speed * dt);
        camera_modified = true;
      }
      if (ImGui::IsKeyDown(ImGuiKey_S)) {
        camera->Translate(0.f, 0.f, -speed * dt);
        camera_modified = true;
      }
      if (ImGui::IsKeyDown(ImGuiKey_A)) {
        camera->Translate(speed * dt, 0.f);
        camera_modified = true;
      }
      if (ImGui::IsKeyDown(ImGuiKey_D)) {
        camera->Translate(-speed * dt, 0.f);
        camera_modified = true;
      }
      if (ImGui::IsKeyDown(ImGuiKey_Space)) {
        camera->Translate(0.f, speed * dt);
        camera_modified = true;
      }
    }

    camera->Update(io.DeltaTime);

    static glm::mat4 model(1.f);
    static bool vsync = true;
    static int sh_degree = splats_->sh_degree();
    static int render_type = 0;
    static glm::vec3 background(0.f, 0.f, 0.f);
    static float camera_scale = 0.1f;
    static int camera_index = 0;
    static bool animation = false;
    static float animation_time = 0.f;
    static float animation_speed = 30.f;

    if (ImGui::Begin("vkgs viewer")) {
      ImGui::Text("FPS: %.2f", io.Framerate);

      if (ImGui::Checkbox("vsync", &vsync)) {
        if (vsync)
          swapchain->SetPresentMode(VK_PRESENT_MODE_FIFO_KHR);
        else
          swapchain->SetPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
      }

      ImGui::SliderInt("SH degree", &sh_degree, 0, splats_->sh_degree());

      constexpr const char* render_types[] = {"Color", "Alpha", "Depth"};
      ImGui::Combo("Render type", &render_type, render_types, IM_ARRAYSIZE(render_types));

      ImGui::ColorEdit3("Background", glm::value_ptr(background));

      if (!camera_params_.empty()) {
        ImGui::SliderFloat("Camera Scale", &camera_scale, 0.01f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic);

        if (ImGui::SliderInt("Camera Index", &camera_index, 0, camera_params_.size() - 1)) {
          const auto& camera_params = camera_params_[camera_index];
          camera->SetView(OpenCVExtrinsicToView(camera_params.extrinsic));
          camera_modified = true;
        }
      }

      if (camera_modified) {
        animation = false;
      }

      if (ImGui::Checkbox("Animation", &animation)) {
        if (animation) {
          animation_time = camera_index;
        }
      }

      if (animation) {
        animation_time += io.DeltaTime * animation_speed;
        animation_time = std::fmod(animation_time, camera_params_.size());

        if (!camera_params_.empty()) {
          camera_index = static_cast<int>(animation_time);
          const auto& camera_params = camera_params_[camera_index];
          camera->SetView(OpenCVExtrinsicToView(camera_params.extrinsic));
        }
      }

      ImGui::SliderFloat("Animation Speed (FPS)", &animation_speed, 1.f, 60.f, "%.2f");
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
      storage.Update(splats_->size(), width, height);
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
        renderer_->ComputeScreenSplats(cb, splats_, draw_options, screen_splats);

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
        // TODO: draw scene to depth image
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
            .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0, 1})
            .Bind(*color_pipeline)
            .Commit(cb);

        ColorPushConstants color_push_constants = {};
        color_push_constants.projection = camera->ProjectionMatrix();
        color_push_constants.view = camera->ViewMatrix();
        for (const auto& camera_param : camera_params_) {
          glm::mat4 ndc_to_image = glm::mat4(1.f);
          ndc_to_image[0][0] = 0.5f * camera_param.width;
          ndc_to_image[1][1] = 0.5f * camera_param.height;
          ndc_to_image[2][0] = 0.5f * camera_param.width;
          ndc_to_image[2][1] = 0.5f * camera_param.height;
          color_push_constants.model = model * glm::inverse(camera_param.extrinsic) *
                                       glm::inverse(glm::mat4(camera_param.intrinsic)) * ndc_to_image *
                                       glm::mat4(glm::mat3(camera_scale));

          gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
              .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants), &color_push_constants)
              .Commit(cb);

          vkCmdBindIndexBuffer(cb, *camera_indices, 0, VK_INDEX_TYPE_UINT32);
          VkBuffer buffer = *camera_vertices;
          VkDeviceSize offset = 0;
          vkCmdBindVertexBuffers(cb, 0, 1, &buffer, &offset);
          vkCmdDrawIndexed(cb, camera_index_size, 1, 0, 0, 0);
        }

        // render splats
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout)
            .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0, render_type == 2 ? 1 : VK_ATTACHMENT_UNUSED})
            .Commit(cb);
        renderer_->RenderScreenSplats(cb, splats_, draw_options, screen_splats, formats,
                                      {VK_ATTACHMENT_UNUSED, 0, render_type == 2 ? 1 : VK_ATTACHMENT_UNUSED},
                                      depth_format, render_type == 2);

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
  camera_vertices = nullptr;
  camera_indices = nullptr;
  device = nullptr;

  if (own_renderer) renderer_ = nullptr;

  glfwDestroyWindow(window_);
}

}  // namespace viewer
}  // namespace vkgs
