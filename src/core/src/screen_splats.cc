#include "vkgs/core/screen_splats.h"

#include <glm/glm.hpp>

#include "vkgs/gpu/buffer.h"
#include "vkgs/core/stats.h"

namespace vkgs {
namespace core {

class ScreenSplatsImpl {
 public:
  void __init__() {
    visible_point_count_ = gpu::Buffer::Create(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        sizeof(uint32_t));
    draw_indirect_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                         sizeof(VkDrawIndexedIndirectCommand));
    stats_ = gpu::Buffer::Create(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        sizeof(Stats));
  }

  void SetProjection(const glm::mat4& projection) { projection_ = projection; }
  void SetIndexBuffer(gpu::Buffer index_buffer) { index_buffer_ = index_buffer; }

  auto visible_point_count() const noexcept { return visible_point_count_; }
  auto projection() const noexcept { return projection_; }
  auto draw_indirect() const noexcept { return draw_indirect_; }
  auto instances() const noexcept { return instances_; }
  auto index_buffer() const noexcept { return index_buffer_; }
  auto stats() const noexcept { return stats_; }

  // Internal
  void Update(uint32_t point_count) {
    if (point_count_ < point_count) {
      instances_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 12 * sizeof(float));
      point_count_ = point_count;
    }
  }

 private:
  uint32_t point_count_ = 0;
  glm::mat4 projection_;

  // Fixed
  gpu::Buffer visible_point_count_;  // (1)
  gpu::Buffer draw_indirect_;        // (DrawIndirect)
  gpu::Buffer stats_;                // (Stats)

  // Variable
  gpu::Buffer instances_;  // (N, 12)

  // Shared
  gpu::Buffer index_buffer_;  // (6N)
};

ScreenSplats ScreenSplats::Create() { return Make<ScreenSplatsImpl>(); }

void ScreenSplats::SetProjection(const glm::mat4& projection) { impl_->SetProjection(projection); }
void ScreenSplats::SetIndexBuffer(gpu::Buffer index_buffer) { impl_->SetIndexBuffer(index_buffer); }

gpu::Buffer ScreenSplats::visible_point_count() const { return impl_->visible_point_count(); }
glm::mat4 ScreenSplats::projection() const { return impl_->projection(); }
gpu::Buffer ScreenSplats::draw_indirect() const { return impl_->draw_indirect(); }
gpu::Buffer ScreenSplats::instances() const { return impl_->instances(); }
gpu::Buffer ScreenSplats::index_buffer() const { return impl_->index_buffer(); }
gpu::Buffer ScreenSplats::stats() const { return impl_->stats(); }

void ScreenSplats::Update(uint32_t point_count) { impl_->Update(point_count); }

}  // namespace core
}  // namespace vkgs
