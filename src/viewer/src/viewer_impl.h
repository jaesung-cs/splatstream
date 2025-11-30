#ifndef VKGS_VIEWER_VIEWER_IMPL_H
#define VKGS_VIEWER_VIEWER_IMPL_H

#include "vkgs/viewer/viewer.h"

#include <memory>
#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/gpu/swapchain.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/graphics_pipeline.h"
#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/semaphore.h"

#include "storage.h"
#include "camera.h"
#include "pose_spline.h"

struct GLFWwindow;

namespace vkgs {

namespace viewer {

class Context;

class Viewer::Impl {
 public:
  Impl();
  ~Impl();

  void SetRenderer(std::shared_ptr<core::Renderer> renderer) { renderer_ = renderer; }
  void SetSplats(std::shared_ptr<core::GaussianSplats> splats) { splats_ = splats; }

  void AddCamera(const CameraParams& camera_params) { camera_params_.push_back(camera_params); }

  void Run();

 private:
  void InitializeWindow();
  void FinalizeWindow();
  void DrawUi();
  void Draw(const gpu::PresentImageInfo& present_image_info);

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
    int sh_degree;
    int render_type;
    glm::vec3 background;
    float camera_scale;
    int camera_index;
    bool animation;
    float animation_time;
    float animation_speed;
    bool camera_modified;
    bool left_panel;
  };
  ViewerOptions viewer_options_ = {};

  Camera camera_;
  PoseSpline pose_spline_;

  std::shared_ptr<core::Renderer> renderer_;
  std::shared_ptr<core::GaussianSplats> splats_;
  std::vector<CameraParams> camera_params_;

  gpu::PipelineLayout color_pipeline_layout_;
  gpu::GraphicsPipeline color_pipeline_;
  gpu::PipelineLayout blend_pipeline_layout_;
  gpu::GraphicsPipeline blend_pipeline_;

  gpu::Buffer camera_vertices_;
  gpu::Buffer camera_indices_;
  uint32_t camera_index_size_ = 0;

  std::array<Storage, 2> ring_buffer_;
  uint64_t frame_index_ = 0;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_IMPL_H
