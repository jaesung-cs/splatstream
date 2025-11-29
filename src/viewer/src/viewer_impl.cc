#include "viewer_impl.h"

#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"  // dock
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/gpu.h"
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

Pose OpenCVExtrinsicToPose(const glm::mat4& extrinsic) {
  auto view = OpenCVExtrinsicToView(extrinsic);
  auto c2w = glm::inverse(view);
  Pose pose;
  pose.q = glm::quat_cast(c2w);
  pose.p = c2w[3];
  return pose;
}

struct ColorPushConstants {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
};

struct BlendPushConstants {
  int mode;
};

}  // namespace

Viewer::Impl::Impl() { context_ = GetContext(); }

Viewer::Impl::~Impl() = default;

void Viewer::Impl::InitializeWindow() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(1280, 720, "vkgs", nullptr, nullptr);
  if (!window_) throw std::runtime_error("Failed to create window");

  auto device = context_->device();
  auto instance = device->instance();
  glfwCreateWindowSurface(instance, window_, NULL, &surface_);

  swapchain_ = std::make_unique<gpu::Swapchain>(surface_, swapchain_format_,
                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  auto gq = device->graphics_queue();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplGlfw_InitForVulkan(window_, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = device->instance();
  init_info.PhysicalDevice = device->physical_device();
  init_info.Device = *device;
  init_info.QueueFamily = *gq;
  init_info.Queue = *gq;
  init_info.DescriptorPoolSize = 1024;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_format_;
  init_info.UseDynamicRendering = true;
  init_info.CheckVkResultFn = nullptr;
  ImGui_ImplVulkan_Init(&init_info);
}

void Viewer::Impl::FinalizeWindow() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
}

void Viewer::Impl::HandleEvents() {
  const auto& io = ImGui::GetIO();

  viewer_options_.camera_modified = false;

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
        camera_.Translate(dx, dy);
        if (dx != 0.f || dy != 0.f) viewer_options_.camera_modified = true;
      } else {
        camera_.Rotate(dx, dy);
        if (dx != 0.f || dy != 0.f) viewer_options_.camera_modified = true;
      }
    } else if (!left && right) {
      camera_.Translate(dx, dy);
      if (dx != 0.f || dy != 0.f) viewer_options_.camera_modified = true;
    } else if (left && right) {
      camera_.Zoom(dy);
      if (dy != 0.f) viewer_options_.camera_modified = true;
    }

    if (io.MouseWheel != 0.f) {
      if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        camera_.DollyZoom(io.MouseWheel);
      } else {
        camera_.Zoom(io.MouseWheel * 10.f);
        viewer_options_.camera_modified = true;
      }
    }
  }

  if (!io.WantCaptureKeyboard) {
    constexpr float speed = 1000.f;
    float dt = io.DeltaTime;
    if (ImGui::IsKeyDown(ImGuiKey_W)) {
      camera_.Translate(0.f, 0.f, speed * dt);
      viewer_options_.camera_modified = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S)) {
      camera_.Translate(0.f, 0.f, -speed * dt);
      viewer_options_.camera_modified = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A)) {
      camera_.Translate(speed * dt, 0.f);
      viewer_options_.camera_modified = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_D)) {
      camera_.Translate(-speed * dt, 0.f);
      viewer_options_.camera_modified = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Space)) {
      camera_.Translate(0.f, speed * dt);
      viewer_options_.camera_modified = true;
    }

    constexpr float pi = 3.14159265f;
    constexpr float rolling_speed = pi;
    if (ImGui::IsKeyDown(ImGuiKey_Q)) {
      camera_.Roll(rolling_speed * dt);
      viewer_options_.camera_modified = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_E)) {
      camera_.Roll(-rolling_speed * dt);
      viewer_options_.camera_modified = true;
    }
  }

  camera_.Update(io.DeltaTime);
}

void Viewer::Impl::DrawUi() {
  const auto& io = ImGui::GetIO();

  // Dock panels
  ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
    ImGuiID dock_id_left;
    ImGuiID dock_id_right;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, &dock_id_left, &dock_id_right);
    auto left_node = ImGui::DockBuilderGetNode(dock_id_left);
    left_node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
    ImGui::DockBuilderDockWindow("Left", dock_id_left);
    ImGui::DockBuilderDockWindow("Right", dock_id_right);
    ImGui::DockBuilderFinish(dockspace_id);
  }

  ImGui::DockSpaceOverViewport(dockspace_id, NULL,
                               ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);

  const ImGuiWindowFlags dock_flags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
  bool left_panel = viewer_options_.left_panel;

  if (left_panel) {
    ImGui::Begin("Left", NULL, dock_flags);
    auto size = ImGui::GetContentRegionAvail();

    ImGui::Text("FPS: %.2f", io.Framerate);
    if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::Checkbox("vsync", &viewer_options_.vsync)) {
        if (viewer_options_.vsync)
          swapchain_->SetPresentMode(VK_PRESENT_MODE_FIFO_KHR);
        else
          swapchain_->SetPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
      }

      ImGui::SliderInt("SH degree", &viewer_options_.sh_degree, 0, splats_->sh_degree());

      constexpr const char* render_types[] = {"Color", "Alpha", "Depth"};
      ImGui::Combo("Render type", &viewer_options_.render_type, render_types, IM_ARRAYSIZE(render_types));

      ImGui::ColorEdit3("Background", glm::value_ptr(viewer_options_.background));

      if (!camera_params_.empty()) {
        ImGui::SliderFloat("Camera Scale", &viewer_options_.camera_scale, 0.01f, 10.f, "%.3f",
                           ImGuiSliderFlags_Logarithmic);

        if (ImGui::SliderInt("Camera Index", &viewer_options_.camera_index, 0, camera_params_.size() - 1)) {
          const auto& camera_params = camera_params_[viewer_options_.camera_index];
          camera_.SetView(OpenCVExtrinsicToView(camera_params.extrinsic));
          viewer_options_.camera_modified = true;
        }
      }

      if (viewer_options_.camera_modified) {
        viewer_options_.animation = false;
      }
    }

    if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::Checkbox("Animation##Button", &viewer_options_.animation)) {
        if (viewer_options_.animation) {
          viewer_options_.animation_time = viewer_options_.camera_index;
        }
      }

      if (viewer_options_.animation) {
        viewer_options_.animation_time += io.DeltaTime * viewer_options_.animation_speed;
        viewer_options_.animation_time = std::fmod(viewer_options_.animation_time, camera_params_.size());

        if (!camera_params_.empty()) {
          auto pose = pose_spline_.Evaluate(viewer_options_.animation_time);
          glm::mat4 w2c = glm::toMat4(pose.q);
          w2c[3] = glm::vec4(pose.p, 1.f);
          auto c2w = glm::inverse(w2c);
          camera_.SetView(c2w);
        }
      }

      ImGui::SliderFloat("Animation Speed (FPS)", &viewer_options_.animation_speed, 1.f, 60.f, "%.2f");
    }

    ImVec2 pos = {size.x + 5.f, size.y / 2.f};
    ImGui::SetCursorScreenPos(pos);
    if (ImGui::Button("<")) {
      viewer_options_.left_panel = false;
    }

    ImGui::End();
  }

  ImGui::Begin("Right", NULL, dock_flags | ImGuiWindowFlags_NoBackground);
  auto size = ImGui::GetContentRegionAvail();
  if (!left_panel) {
    ImVec2 pos = {-5.f, size.y / 2.f};
    ImGui::SetCursorPos(pos);
    if (ImGui::Button(">")) {
      viewer_options_.left_panel = true;
    }
  }
  ImGui::End();
}

void Viewer::Impl::Run() {
  InitializeWindow();

  // Formats
  std::vector<VkFormat> formats = {swapchain_format_, high_format_, depth_image_format_};

  // Graphics pipelines
  color_pipeline_layout_ =
      gpu::PipelineLayout::Create({}, {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants)}});

  gpu::GraphicsPipelineCreateInfo color_pipeline_info = {};
  color_pipeline_info.pipeline_layout = *color_pipeline_layout_;
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
  color_pipeline_info.depth_format = depth_format_;
  color_pipeline_info.depth_test = true;
  color_pipeline_info.depth_write = true;
  color_pipeline_ = gpu::GraphicsPipeline::Create(color_pipeline_info);

  blend_pipeline_layout_ =
      gpu::PipelineLayout::Create({{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                   {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT}},
                                  {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlendPushConstants)}});

  gpu::GraphicsPipelineCreateInfo blend_pipeline_info = {};
  blend_pipeline_info.pipeline_layout = *blend_pipeline_layout_;
  blend_pipeline_info.vertex_shader = gpu::ShaderCode(blend_vert);
  blend_pipeline_info.fragment_shader = gpu::ShaderCode(blend_frag);
  blend_pipeline_info.formats = formats;
  blend_pipeline_info.locations = {0, VK_ATTACHMENT_UNUSED, VK_ATTACHMENT_UNUSED};
  blend_pipeline_info.input_indices = {VK_ATTACHMENT_UNUSED, 0, 1};
  blend_pipeline_info.depth_format = depth_format_;
  blend_pipeline_ = gpu::GraphicsPipeline::Create(blend_pipeline_info);

  // Camera
  if (!camera_params_.empty()) {
    // Initialize camera with first camera params
    const auto& camera_params = camera_params_[0];
    camera_.SetView(OpenCVExtrinsicToView(camera_params.extrinsic));
    camera_.Update(10.f);
  }

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

    camera_index_size_ = indices.size();

    auto indices_stage =
        gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indices.size() * sizeof(indices[0]), true);
    auto vertices_stage =
        gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertices.size() * sizeof(vertices[0]), true);

    std::memcpy(indices_stage->data(), indices.data(), indices.size() * sizeof(indices[0]));
    std::memcpy(vertices_stage->data(), vertices.data(), vertices.size() * sizeof(vertices[0]));

    camera_vertices_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           vertices.size() * sizeof(vertices[0]));
    camera_indices_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                          indices.size() * sizeof(indices[0]));

    VkBufferCopy region = {0, 0, indices_stage->size()};
    vkCmdCopyBuffer(cb, *indices_stage, *camera_indices_, 1, &region);
    region = {0, 0, vertices_stage->size()};
    vkCmdCopyBuffer(cb, *vertices_stage, *camera_vertices_, 1, &region);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT)
        .Commit(cb);

    indices_stage->Keep();
    vertices_stage->Keep();
    camera_vertices_->Keep();
    camera_indices_->Keep();

    task.Submit();
  }

  // Camera spline
  for (int i = 0; i < camera_params_.size(); i++) {
    const auto& camera_params = camera_params_[i];
    pose_spline_.push_back(OpenCVExtrinsicToPose(camera_params.extrinsic));
  }

  if (!renderer_) renderer_ = std::make_shared<core::Renderer>();

  // Viewer options
  viewer_options_.model = glm::mat4{1.f};
  viewer_options_.vsync = true;
  viewer_options_.sh_degree = splats_->sh_degree();
  viewer_options_.render_type = 0;
  viewer_options_.background = {0.f, 0.f, 0.f};
  viewer_options_.camera_scale = 0.1f;
  viewer_options_.camera_index = 0;
  viewer_options_.animation = false;
  viewer_options_.animation_time = 0.f;
  viewer_options_.animation_speed = 30.f;
  viewer_options_.left_panel = true;

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    HandleEvents();
    DrawUi();

    const auto& io = ImGui::GetIO();
    bool is_minimized = io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f;

    if (is_minimized) {
      ImGui::EndFrame();
    } else {
      auto present_image_info = swapchain_->AcquireNextImage();
      Draw(present_image_info);
      swapchain_->Present();
    }
  }

  context_->device()->WaitIdle();

  FinalizeWindow();
}

void Viewer::Impl::Draw(const gpu::PresentImageInfo& present_image_info) {
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  std::vector<VkFormat> formats = {swapchain_format_, high_format_, depth_image_format_};

  auto cq = context_->device()->compute_queue();
  auto gq = context_->device()->graphics_queue();

  auto width = present_image_info.extent.width;
  auto height = present_image_info.extent.height;

  camera_.SetWindowSize(width, height);

  core::DrawOptions draw_options = {};
  draw_options.view = camera_.ViewMatrix();
  draw_options.projection = camera_.ProjectionMatrix();
  draw_options.model = viewer_options_.model;
  draw_options.width = width;
  draw_options.height = height;
  draw_options.background = {0.f, 0.f, 0.f};  // unused
  draw_options.eps2d = 0.01f;
  draw_options.sh_degree = viewer_options_.render_type == 0 ? viewer_options_.sh_degree : 0;

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
    color_attachments[1].clearValue.color = {viewer_options_.background.r, viewer_options_.background.g,
                                             viewer_options_.background.b, 0.f};
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
    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *color_pipeline_layout_)
        .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0, 1})
        .Bind(*color_pipeline_)
        .Commit(cb);

    ColorPushConstants color_push_constants = {};
    color_push_constants.projection = camera_.ProjectionMatrix();
    color_push_constants.view = camera_.ViewMatrix();
    for (const auto& camera_param : camera_params_) {
      glm::mat4 ndc_to_image = glm::mat4(1.f);
      ndc_to_image[0][0] = 0.5f * camera_param.width;
      ndc_to_image[1][1] = 0.5f * camera_param.height;
      ndc_to_image[2][0] = 0.5f * camera_param.width;
      ndc_to_image[2][1] = 0.5f * camera_param.height;
      color_push_constants.model = viewer_options_.model * glm::inverse(camera_param.extrinsic) *
                                   glm::inverse(glm::mat4(camera_param.intrinsic)) * ndc_to_image *
                                   glm::mat4(glm::mat3(viewer_options_.camera_scale));

      gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *color_pipeline_layout_)
          .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(color_push_constants), &color_push_constants)
          .Commit(cb);

      vkCmdBindIndexBuffer(cb, *camera_indices_, 0, VK_INDEX_TYPE_UINT32);
      VkBuffer buffer = *camera_vertices_;
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cb, 0, 1, &buffer, &offset);
      vkCmdDrawIndexed(cb, camera_index_size_, 1, 0, 0, 0);
    }

    // render splats
    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *color_pipeline_layout_)
        .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0, viewer_options_.render_type == 2 ? 1 : VK_ATTACHMENT_UNUSED})
        .Commit(cb);
    renderer_->RenderScreenSplats(
        cb, splats_, draw_options, screen_splats, formats,
        {VK_ATTACHMENT_UNUSED, 0, viewer_options_.render_type == 2 ? 1 : VK_ATTACHMENT_UNUSED}, depth_format_,
        viewer_options_.render_type == 2);

    // subpass 1:
    gpu::cmd::Barrier(VK_DEPENDENCY_BY_REGION_BIT)
        .Memory(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT)
        .Commit(cb);

    BlendPushConstants blend_push_constants = {};
    blend_push_constants.mode = viewer_options_.render_type;
    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *blend_pipeline_layout_)
        .Input(0, image16->image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
        .Input(1, depth_image->image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
        .PushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(blend_push_constants), &blend_push_constants)
        .Bind(*blend_pipeline_)
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
}

}  // namespace viewer
}  // namespace vkgs
