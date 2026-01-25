#ifndef VKGS_VIEWER_STORAGE_H
#define VKGS_VIEWER_STORAGE_H

#include <memory>

#include "vkgs/core/stats.h"
#include "vkgs/core/screen_splats.h"
#include "vkgs/gpu/sampler.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/queue_task.h"

namespace vkgs {
namespace viewer {

class Storage {
 public:
  Storage();
  ~Storage();

  void Update(uint32_t size, uint32_t width, uint32_t height);
  void Clear();

  auto screen_splats() const noexcept { return screen_splats_; }

  auto visible_point_count_stage() const noexcept { return visible_point_count_stage_; }
  auto stats_stage() const noexcept { return stats_stage_; }

  // Intermediate color image
  auto image16() const noexcept { return image16_; }

  // Depth image
  auto depth_image() const noexcept { return depth_image_; }

  // Depth attachment
  auto depth() const noexcept { return depth_; }

  auto compute_semaphore() const noexcept { return compute_semaphore_; }
  auto graphics_semaphore() const noexcept { return graphics_semaphore_; }

  void SetTask(gpu::QueueTask task) { task_ = task; }
  void Wait() {
    if (task_) task_.Wait();
  }

  void SetVisiblePointCount(uint32_t visible_point_count) noexcept { visible_point_count_ = visible_point_count; }

  auto visible_point_count() const noexcept { return visible_point_count_; }
  const auto& stats() const noexcept { return stats_; }
  auto& stats() noexcept { return stats_; }

 private:
  core::ScreenSplats screen_splats_;
  gpu::Buffer visible_point_count_stage_;
  gpu::Buffer stats_stage_;
  gpu::Image image16_;
  gpu::Image depth_image_;
  gpu::Image depth_;
  gpu::Semaphore compute_semaphore_;
  gpu::Semaphore graphics_semaphore_;

  // Draw result
  gpu::QueueTask task_;
  uint32_t visible_point_count_ = 0;
  core::Stats stats_{};
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_STORAGE_H
