#include "vkgs/viewer/viewer.h"

#include <memory>
#include <array>
#include <vector>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include "implot.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "vkgs/common/timer.h"
#include "vkgs/common/sampled_moving_window.h"
#include "vkgs/gpu/swapchain.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/graphics_pipeline.h"
#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/task.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/timer.h"
#include "vkgs/gpu/cmd/pipeline.h"
#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/renderer.h"
#include "vkgs/core/screen_splats.h"
#include "vkgs/core/stats.h"
#include "vkgs/core/draw_options.h"

#include "vkgs/viewer/camera_params.h"
#include "generated/color_vert.h"
#include "generated/color_frag.h"
#include "generated/depth_vert.h"
#include "generated/depth_frag.h"
#include "generated/screen_vert.h"
#include "generated/blend_color_frag.h"
#include "generated/blend_depth_frag.h"
#include "fonts/roboto_regular.h"
#include "context.h"
#include "storage.h"
#include "camera.h"
#include "pose_spline.h"

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
  int gamma_correction;
};

}  // namespace

class ViewerImpl {
 public:
  void __init__() { context_ = GetContext(); }

  void SetRenderer(core::Renderer renderer) { renderer_ = renderer; }
  void SetSplats(core::GaussianSplats splats) { splats_ = splats; }

  void AddCamera(const CameraParams& camera_params) { camera_params_.push_back(camera_params); }
  void ClearCameras() { camera_params_.clear(); }

  void Run() {
    InitializeWindow();

    // Graphics pipelines
    color_pipeline_layout_ = gpu::PipelineLayout::Create({
        .push_constants = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants)}},
    });

    color_pipeline_ = gpu::GraphicsPipeline::Create({
        .pipeline_layout = color_pipeline_layout_,
        .vertex_shader = gpu::ShaderCode(color_vert),
        .fragment_shader = gpu::ShaderCode(color_frag),
        .bindings = {{0, sizeof(float) * 6, VK_VERTEX_INPUT_RATE_VERTEX}},
        .attributes =
            {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 0},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3},
            },
        .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        .formats = {swapchain_format_, high_format_},
        .locations = {VK_ATTACHMENT_UNUSED, 0},
        .depth_format = depth_format_,
        .depth_test = true,
        .depth_write = true,
    });
    depth_pipeline_ = gpu::GraphicsPipeline::Create({
        .pipeline_layout = color_pipeline_layout_,
        .vertex_shader = gpu::ShaderCode(depth_vert),
        .fragment_shader = gpu::ShaderCode(depth_frag),
        .bindings = {{0, sizeof(float) * 6, VK_VERTEX_INPUT_RATE_VERTEX}},
        .attributes = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 0}},
        .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        .formats = {swapchain_format_, depth_image_format_},
        .locations = {VK_ATTACHMENT_UNUSED, 0},
        .depth_format = depth_format_,
        .depth_test = true,
        .depth_write = true,
    });

    blend_pipeline_layout_ = gpu::PipelineLayout::Create({
        .bindings = {{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                     {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT}},
        .push_constants = {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlendPushConstants)}},
    });

    blend_color_pipeline_ = gpu::GraphicsPipeline::Create({
        .pipeline_layout = blend_pipeline_layout_,
        .vertex_shader = gpu::ShaderCode(screen_vert),
        .fragment_shader = gpu::ShaderCode(blend_color_frag),
        .formats = {swapchain_format_, high_format_},
        .locations = {0, VK_ATTACHMENT_UNUSED},
        .input_indices = {VK_ATTACHMENT_UNUSED, 0},
        .depth_format = depth_format_,
    });
    blend_depth_pipeline_ = gpu::GraphicsPipeline::Create({
        .pipeline_layout = blend_pipeline_layout_,
        .vertex_shader = gpu::ShaderCode(screen_vert),
        .fragment_shader = gpu::ShaderCode(blend_depth_frag),
        .formats = {swapchain_format_, depth_image_format_},
        .locations = {0, VK_ATTACHMENT_UNUSED},
        .input_indices = {VK_ATTACHMENT_UNUSED, 0},
        .depth_format = depth_format_,
    });

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

      auto indices_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indices.size() * sizeof(indices[0]));
      auto vertices_stage =
          gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertices.size() * sizeof(vertices[0]));

      std::memcpy(indices_stage.data(), indices.data(), indices.size() * sizeof(indices[0]));
      std::memcpy(vertices_stage.data(), vertices.data(), vertices.size() * sizeof(vertices[0]));

      camera_vertices_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                             vertices.size() * sizeof(vertices[0]));
      camera_indices_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            indices.size() * sizeof(indices[0]));

      VkBufferCopy region = {0, 0, indices_stage.size()};
      vkCmdCopyBuffer(cb, indices_stage, camera_indices_, 1, &region);
      region = {0, 0, vertices_stage.size()};
      vkCmdCopyBuffer(cb, vertices_stage, camera_vertices_, 1, &region);

      gpu::cmd::Barrier()
          .Memory(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                  VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT)
          .Commit(cb);

      indices_stage.Keep();
      vertices_stage.Keep();
      camera_vertices_.Keep();
      camera_indices_.Keep();

      task.Submit();
    }

    // Camera spline
    pose_spline_.clear();
    for (int i = 0; i < camera_params_.size(); i++) {
      const auto& camera_params = camera_params_[i];
      pose_spline_.push_back(OpenCVExtrinsicToPose(camera_params.extrinsic));
    }

    if (!renderer_) renderer_ = core::Renderer::Create();

    // Viewer options
    viewer_options_ = {
        .model = glm::mat4{1.f},
        .vsync = true,
        .gamma_correction = false,
        .sh_degree = static_cast<int>(splats_.sh_degree()),
        .render_type = 0,
        .background = {0.f, 0.f, 0.f},
        .eps2d = 0.01f,
        .confidence_radius = 3.5f,
        .show_camera_frames = false,
        .camera_frame_scale = 0.1f,
        .camera_index = 0,
        .animation = false,
        .animation_time = 0.f,
        .animation_speed = 1.f,
        .show_stat = true,
    };

    timer_.start();
    fps_window_ = SampledMovingWindow(5.f, 0.01f);
    visible_point_count_window_ = SampledMovingWindow(5.f, 0.01f);
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
        auto present_image_info = swapchain_.AcquireNextImage();
        Draw(present_image_info);
        swapchain_.Present();
      }
    }

    context_->device().WaitIdle();

    for (auto& storage : ring_buffer_) storage.Clear();

    FinalizeWindow();
  }

 private:
  void InitializeWindow() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(1600, 900, "vkgs", nullptr, nullptr);
    if (!window_) throw std::runtime_error("Failed to create window");

    auto device = context_->device();
    auto instance = device.instance();
    glfwCreateWindowSurface(instance, window_, NULL, &surface_);

    swapchain_ = gpu::Swapchain::Create(surface_, swapchain_format_,
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    auto gq = device.graphics_queue();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    ImPlot::CreateContext();

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

  void FinalizeWindow() {
    ImPlot::DestroyContext();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
  }

  void DrawUi() {
    const auto& io = ImGui::GetIO();

    fps_window_.add(timer_.elapsed(), io.Framerate);

    viewer_options_.camera_modified = false;

    if (!io.WantCaptureMouse) {
      bool left = ImGui::IsMouseDown(ImGuiMouseButton_Left);
      bool right = ImGui::IsMouseDown(ImGuiMouseButton_Right);
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

    auto& storage = ring_buffer_[frame_index_ % ring_buffer_.size()];
    storage.Wait();
    auto point_count = static_cast<int>(splats_.size());
    auto visible_point_count = static_cast<int>(storage.visible_point_count());
    const auto& stats = storage.stats();

    visible_point_count_window_.add(timer_.elapsed(), visible_point_count);

    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({300, 0});
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin("Settings", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
      if (ImGui::CollapsingHeader("Graphic")) {
        if (ImGui::Checkbox("V-Sync", &viewer_options_.vsync)) {
          if (viewer_options_.vsync)
            swapchain_.SetPresentMode(VK_PRESENT_MODE_FIFO_KHR);
          else
            swapchain_.SetPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
        }

        ImGui::Checkbox("Gamma Correction", &viewer_options_.gamma_correction);
      }

      if (ImGui::CollapsingHeader("Scene")) {
        constexpr const char* render_types[] = {"Color", "Alpha", "Depth (experimental)"};
        ImGui::Combo("Render Type", &viewer_options_.render_type, render_types, IM_ARRAYSIZE(render_types));

        ImGui::ColorEdit3("Background", glm::value_ptr(viewer_options_.background));

        ImGui::SliderInt("SH degree", &viewer_options_.sh_degree, 0, splats_.sh_degree());
        ImGui::SliderFloat("Eps2d", &viewer_options_.eps2d, 0.0001f, 10.f, "%.4f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Radius", &viewer_options_.confidence_radius, 1.f, 5.f, "%.3f");
      }

      if (!camera_params_.empty()) {
        if (ImGui::CollapsingHeader("Camera")) {
          ImGui::Checkbox("Show Camera Frames", &viewer_options_.show_camera_frames);
          ImGui::SliderFloat("Frame Size", &viewer_options_.camera_frame_scale, 0.01f, 10.f, "%.3f",
                             ImGuiSliderFlags_Logarithmic);

          if (ImGui::SliderInt("Index", &viewer_options_.camera_index, 0, camera_params_.size() - 1)) {
            const auto& camera_params = camera_params_[viewer_options_.camera_index];
            camera_.SetView(OpenCVExtrinsicToView(camera_params.extrinsic));
            viewer_options_.camera_modified = true;
            viewer_options_.animation_time = viewer_options_.camera_index;
          }
        }

        if (ImGui::CollapsingHeader("Animation")) {
          ImGui::Checkbox("Animation##Button", &viewer_options_.animation);
          ImGui::SliderFloat("Speed (FPS)", &viewer_options_.animation_speed, 1.f, 30.f, "%.2f");
        }
      }
    }
    ImGui::End();

    ImGui::SetNextWindowPos({io.DisplaySize.x - 10, 10}, ImGuiCond_Always, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowSize({300, 0});
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin("Info", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
      const auto& display_size = io.DisplaySize;
      ImGui::Text("FPS: %.2f", io.Framerate);
      ImGui::Text("Resolution: %dx%d", (int)display_size.x, (int)display_size.y);
      ImGui::Text("Point count   : %d", point_count);
      ImGui::Text("Visible points: %d (%.2f%%)", visible_point_count,
                  static_cast<float>(visible_point_count) / point_count * 100.f);

      if (ImPlot::BeginPlot("FPS", {-1, 100}, ImPlotFlags_NoLegend)) {
        std::vector<float> xs;
        std::vector<float> ys;
        float y_max = 0.f;
        for (const auto& point : fps_window_.points()) {
          xs.push_back(point.time);
          ys.push_back(point.value);
          y_max = std::max(y_max, point.value);
        }
        float y_margin = y_max * 0.05f;
        ImPlot::SetupAxis(ImAxis_X1, "Time",
                          ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxis(ImAxis_Y1, "FPS");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.f - y_margin, y_max + y_margin, ImPlotCond_Always);
        ImPlot::PlotLine("FPS", xs.data(), ys.data(), xs.size());
        ImPlot::EndPlot();
      }

      if (ImPlot::BeginPlot("Visible Points (%)", {-1, 100}, ImPlotFlags_NoLegend)) {
        std::vector<float> xs;
        std::vector<float> ys;
        for (const auto& point : visible_point_count_window_.points()) {
          xs.push_back(point.time);
          ys.push_back(point.value / point_count * 100.f);
        }
        float y_max = 100.f;
        float y_margin = y_max * 0.05f;
        ImPlot::SetupAxis(ImAxis_X1, "Time",
                          ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxis(ImAxis_Y1, "Visible (%)");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.f - y_margin, y_max + y_margin, ImPlotCond_Always);
        ImPlot::PlotLine("Visible Points (%)", xs.data(), ys.data(), xs.size());
        ImPlot::EndPlot();
      }

      if (ImGui::CollapsingHeader("Statistics (Slightly Decrease FPS!)")) {
        viewer_options_.show_stat = true;
        if (ImPlot::BeginPlot("Splat Alpha Histogram", {-1, 200}, ImPlotFlags_NoLegend)) {
          std::array<float, 50> xs;
          for (int i = 0; i < 50; ++i) xs[i] = i / 50.f;
          std::array<float, 50> ys;
          for (int i = 0; i < 50; ++i) ys[i] = stats.histogram_alpha[i];
          ImPlot::SetupAxis(ImAxis_X1, "Alpha", ImPlotAxisFlags_AutoFit);
          ImPlot::SetupAxis(ImAxis_Y1, "Count", ImPlotAxisFlags_AutoFit);
          ImPlot::PlotBars("Splat Alpha", xs.data(), ys.data(), xs.size(), (xs[1] - xs[0]) * 0.67f);
          ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("Projection Active Threads", {-1, 200}, ImPlotFlags_NoLegend)) {
          std::array<float, 64> xs;
          for (int i = 0; i < 64; ++i) xs[i] = i + 1;
          std::array<float, 64> ys;
          for (int i = 0; i < 64; ++i) ys[i] = stats.histogram_projection_active_threads[i];
          ImPlot::SetupAxis(ImAxis_X1, "Threads", ImPlotAxisFlags_AutoFit);
          ImPlot::SetupAxis(ImAxis_Y1, "Count", ImPlotAxisFlags_AutoFit);
          ImPlot::PlotBars("Active Threads", xs.data(), ys.data(), xs.size(), (xs[1] - xs[0]) * 0.67f);
          ImPlot::EndPlot();
        }
      } else {
        viewer_options_.show_stat = false;
      }
    }
    ImGui::End();

    if (viewer_options_.camera_modified) {
      viewer_options_.animation = false;
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
  }

  void Draw(const gpu::PresentImageInfo& present_image_info) {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    auto cq = context_->device().compute_queue();
    auto gq = context_->device().graphics_queue();

    auto texture_width = present_image_info.extent.width;
    auto texture_height = present_image_info.extent.height;

    // ring buffer
    auto& storage = ring_buffer_[frame_index_ % ring_buffer_.size()];
    storage.Update(splats_.size(), texture_width, texture_height);
    auto screen_splats = storage.screen_splats();
    auto csem = storage.compute_semaphore();
    auto gsem = storage.graphics_semaphore();
    auto image16 = storage.image16();
    auto depth = storage.depth();
    auto depth_image = storage.depth_image();
    auto visible_point_count_stage = storage.visible_point_count_stage();
    auto stats_stage = storage.stats_stage();
    auto& stats = storage.stats();
    frame_index_++;

    camera_.SetWindowSize(texture_width, texture_height);

    core::DrawOptions draw_options = {
        .view = camera_.ViewMatrix(),
        .projection = camera_.ProjectionMatrix(),
        .model = viewer_options_.model,
        .width = texture_width,
        .height = texture_height,
        .background = {0.f, 0.f, 0.f},  // unused
        .eps2d = viewer_options_.eps2d,
        .sh_degree = viewer_options_.render_type == 0 ? viewer_options_.sh_degree : 0,
        .record_stat = viewer_options_.show_stat,
    };

    // Compute queue
    {
      gpu::ComputeTask task;
      auto cb = task.command_buffer();

      renderer_.ComputeScreenSplats(cb, splats_, draw_options, screen_splats, {});

      // Get stats to stage buffer
      gpu::cmd::Barrier()
          .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT)
          .Commit(cb);

      VkBufferCopy region = {0, 0, sizeof(uint32_t)};
      vkCmdCopyBuffer(cb, screen_splats.visible_point_count(), visible_point_count_stage, 1, &region);
      if (draw_options.record_stat) {
        region = {0, 0, sizeof(core::Stats)};
        vkCmdCopyBuffer(cb, screen_splats.stats(), stats_stage, 1, &region);
      }

      // Release
      gpu::cmd::Barrier()
          .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, cq, gq,
                   screen_splats.instances())
          .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, cq, gq,
                   screen_splats.draw_indirect())
          .Commit(cb);

      screen_splats.visible_point_count().Keep();
      visible_point_count_stage.Keep();
      screen_splats.stats().Keep();
      stats_stage.Keep();

      task.Signal(csem, csem + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

      task.PostCallback([visible_point_count_stage, stats_stage, stats = &stats, storage = &storage,
                         record_stat = draw_options.record_stat] {
        uint32_t visible_point_count;
        std::memcpy(&visible_point_count, visible_point_count_stage.data(), sizeof(uint32_t));
        storage->SetVisiblePointCount(visible_point_count);

        if (record_stat) {
          std::memcpy(stats, stats_stage.data(), sizeof(core::Stats));
        } else {
          std::memset(stats, 0, sizeof(core::Stats));
        }
      });

      auto compute_task = task.Submit();
      storage.SetTask(compute_task);
    }

    // Graphics queue
    {
      gpu::GraphicsTask task;
      auto cb = task.command_buffer();

      gpu::cmd::Barrier()
          // Acquire
          .Acquire(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, cq, gq,
                   screen_splats.instances())
          .Acquire(VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, cq, gq,
                   screen_splats.draw_indirect())
          // Image layout transition
          .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, present_image_info.image)
          .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, image16)
          .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, depth_image)
          .Image(0, 0, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depth)
          .Commit(cb);

      // Rendering
      std::vector<VkFormat> formats;
      std::array<VkRenderingAttachmentInfo, 2> color_attachments;
      // Swapchain image
      color_attachments[0] = {
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .imageView = present_image_info.image_view,
          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .clearValue = {0.f, 0.f, 0.f, 1.f},
      };
      if (viewer_options_.render_type == 0 || viewer_options_.render_type == 1) {
        // Splats image
        formats = {swapchain_format_, image16.format()};
        color_attachments[1] = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = image16.image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {viewer_options_.background.r, viewer_options_.background.g, viewer_options_.background.b,
                           0.f},
        };
      } else if (viewer_options_.render_type == 2) {
        // Depth image
        formats = {swapchain_format_, depth_image.format()};
        color_attachments[1] = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = depth_image.image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {0.f, 0.f, 0.f, 0.f},
        };
      }

      VkRenderingAttachmentInfo depth_attachment = {
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .imageView = depth.image_view(),
          .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .clearValue = {1.f, 0},
      };

      VkRenderingInfo rendering_info = {
          .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
          .renderArea = {{0, 0}, {texture_width, texture_height}},
          .layerCount = 1,
          .colorAttachmentCount = color_attachments.size(),
          .pColorAttachments = color_attachments.data(),
          .pDepthAttachment = &depth_attachment,
      };
      vkCmdBeginRendering(cb, &rendering_info);

      VkViewport viewport = {0.f, 0.f, static_cast<float>(texture_width), static_cast<float>(texture_height), 0.f, 1.f};
      vkCmdSetViewport(cb, 0, 1, &viewport);
      VkRect2D scissor = {0, 0, texture_width, texture_height};
      vkCmdSetScissor(cb, 0, 1, &scissor);

      // render scene
      if (viewer_options_.render_type == 0 || viewer_options_.render_type == 1) {
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, color_pipeline_layout_)
            .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0})
            .Bind(color_pipeline_)
            .Commit(cb);

        if (viewer_options_.show_camera_frames) {
          ColorPushConstants color_push_constants = {
              .projection = camera_.ProjectionMatrix(),
              .view = camera_.ViewMatrix(),
          };
          for (const auto& camera_param : camera_params_) {
            glm::mat4 ndc_to_image = glm::mat4(1.f);
            ndc_to_image[0][0] = 0.5f * camera_param.width;
            ndc_to_image[1][1] = 0.5f * camera_param.height;
            ndc_to_image[2][0] = 0.5f * camera_param.width;
            ndc_to_image[2][1] = 0.5f * camera_param.height;
            color_push_constants.model = viewer_options_.model * glm::inverse(camera_param.extrinsic) *
                                         glm::inverse(glm::mat4(camera_param.intrinsic)) * ndc_to_image *
                                         glm::mat4(glm::mat3(viewer_options_.camera_frame_scale));

            gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, color_pipeline_layout_)
                .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(color_push_constants), &color_push_constants)
                .Commit(cb);

            vkCmdBindIndexBuffer(cb, camera_indices_, 0, VK_INDEX_TYPE_UINT32);
            VkBuffer buffer = camera_vertices_;
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cb, 0, 1, &buffer, &offset);
            vkCmdDrawIndexed(cb, camera_index_size_, 1, 0, 0, 0);
          }
        }
      } else if (viewer_options_.render_type == 2) {
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, color_pipeline_layout_)
            .AttachmentLocations({VK_ATTACHMENT_UNUSED, 0})
            .Bind(depth_pipeline_)
            .Commit(cb);

        if (viewer_options_.show_camera_frames) {
          ColorPushConstants color_push_constants = {
              .projection = camera_.ProjectionMatrix(),
              .view = camera_.ViewMatrix(),
          };
          for (const auto& camera_param : camera_params_) {
            glm::mat4 ndc_to_image = glm::mat4(1.f);
            ndc_to_image[0][0] = 0.5f * camera_param.width;
            ndc_to_image[1][1] = 0.5f * camera_param.height;
            ndc_to_image[2][0] = 0.5f * camera_param.width;
            ndc_to_image[2][1] = 0.5f * camera_param.height;
            color_push_constants.model = viewer_options_.model * glm::inverse(camera_param.extrinsic) *
                                         glm::inverse(glm::mat4(camera_param.intrinsic)) * ndc_to_image *
                                         glm::mat4(glm::mat3(viewer_options_.camera_frame_scale));

            gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, color_pipeline_layout_)
                .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(color_push_constants), &color_push_constants)
                .Commit(cb);

            vkCmdBindIndexBuffer(cb, camera_indices_, 0, VK_INDEX_TYPE_UINT32);
            VkBuffer buffer = camera_vertices_;
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cb, 0, 1, &buffer, &offset);
            vkCmdDrawIndexed(cb, camera_index_size_, 1, 0, 0, 0);
          }
        }
      }

      // render splats
      std::vector<uint32_t> locations = {VK_ATTACHMENT_UNUSED, 0};
      gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, color_pipeline_layout_)
          .AttachmentLocations(locations)
          .Commit(cb);

      core::ScreenSplatOptions screen_splat_options = {
          .confidence_radius = viewer_options_.confidence_radius,
      };
      core::RenderTargetOptions render_target_options = {
          .formats = formats,
          .locations = locations,
          .depth_format = depth_format_,
      };
      if (viewer_options_.render_type == 0 || viewer_options_.render_type == 1) {
        renderer_.RenderScreenSplatsColor(cb, screen_splats, screen_splat_options, render_target_options);
      } else if (viewer_options_.render_type == 2) {
        renderer_.RenderScreenSplatsDepth(cb, screen_splats, screen_splat_options, render_target_options);
      }

      // subpass 1: blend
      gpu::cmd::Barrier(VK_DEPENDENCY_BY_REGION_BIT)
          .Memory(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT)
          .Commit(cb);

      BlendPushConstants blend_push_constants = {
          .mode = viewer_options_.render_type,
          .gamma_correction = viewer_options_.gamma_correction,
      };
      if (viewer_options_.render_type == 0 || viewer_options_.render_type == 1) {
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, blend_pipeline_layout_)
            .Input(0, image16.image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
            .PushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(blend_push_constants), &blend_push_constants)
            .Bind(blend_color_pipeline_)
            .AttachmentLocations({0, VK_ATTACHMENT_UNUSED})
            .InputAttachmentIndices({VK_ATTACHMENT_UNUSED, 0})
            .Commit(cb);
        vkCmdDraw(cb, 3, 1, 0, 0);
      } else if (viewer_options_.render_type == 2) {
        gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, blend_pipeline_layout_)
            .Input(0, depth_image.image_view(), VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ)
            .Bind(blend_depth_pipeline_)
            .AttachmentLocations({0, VK_ATTACHMENT_UNUSED})
            .InputAttachmentIndices({VK_ATTACHMENT_UNUSED, 0})
            .Commit(cb);
        vkCmdDraw(cb, 3, 1, 0, 0);
      }

      vkCmdEndRendering(cb);

      // Rendering UI
      VkRenderingAttachmentInfo color_attachment = {
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .imageView = present_image_info.image_view,
          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      };
      rendering_info = {
          .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
          .renderArea = {{0, 0}, present_image_info.extent},
          .layerCount = 1,
          .colorAttachmentCount = 1,
          .pColorAttachments = &color_attachment,
      };
      vkCmdBeginRendering(cb, &rendering_info);
      ImGui_ImplVulkan_RenderDrawData(draw_data, cb);
      vkCmdEndRendering(cb);

      gpu::cmd::Barrier()
          .Image(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0, 0,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, present_image_info.image)
          .Commit(cb);

      image16.Keep();
      depth_image.Keep();
      depth.Keep();

      task.Wait(present_image_info.image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
      task.Wait(csem, csem + 1,
                VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
      task.Signal(gsem, gsem + 1, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
      task.Signal(present_image_info.render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    csem++;
    gsem++;
  }

  std::shared_ptr<Context> context_;

  GLFWwindow* window_ = nullptr;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_UNORM;
  VkFormat high_format_ = VK_FORMAT_R16G16B16A16_SFLOAT;
  VkFormat depth_image_format_ = VK_FORMAT_R16G16_SFLOAT;
  VkFormat depth_format_ = VK_FORMAT_D32_SFLOAT;
  gpu::Swapchain swapchain_;

  struct ViewerOptions {
    glm::mat4 model;
    bool vsync;
    bool gamma_correction;
    int sh_degree;
    int render_type;
    glm::vec3 background;
    float eps2d;
    float confidence_radius;
    bool show_camera_frames;
    float camera_frame_scale;
    int camera_index;
    bool animation;
    float animation_time;
    float animation_speed;
    bool camera_modified;
    bool show_stat;
  };
  ViewerOptions viewer_options_ = {};

  Camera camera_;
  PoseSpline pose_spline_;

  core::Renderer renderer_;
  core::GaussianSplats splats_;
  std::vector<CameraParams> camera_params_;

  gpu::PipelineLayout color_pipeline_layout_;
  gpu::GraphicsPipeline color_pipeline_;
  gpu::GraphicsPipeline depth_pipeline_;
  gpu::PipelineLayout blend_pipeline_layout_;
  gpu::GraphicsPipeline blend_color_pipeline_;
  gpu::GraphicsPipeline blend_depth_pipeline_;

  gpu::Buffer camera_vertices_;
  gpu::Buffer camera_indices_;
  uint32_t camera_index_size_ = 0;

  std::array<Storage, 2> ring_buffer_;
  uint64_t frame_index_ = 0;
  Timer timer_;
  SampledMovingWindow fps_window_;
  SampledMovingWindow visible_point_count_window_;
};

Viewer Viewer::Create() { return Make<ViewerImpl>(); }

void Viewer::SetRenderer(core::Renderer renderer) { impl_->SetRenderer(renderer); }
void Viewer::SetSplats(core::GaussianSplats splats) { impl_->SetSplats(splats); }
void Viewer::AddCamera(const CameraParams& camera_params) { impl_->AddCamera(camera_params); }
void Viewer::ClearCameras() { impl_->ClearCameras(); }
void Viewer::Run() { impl_->Run(); }

}  // namespace viewer
}  // namespace vkgs
