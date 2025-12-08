#include "vkgs/core/screen_splats.h"

#include "vkgs/gpu/buffer.h"

#include "vkgs/core/stats.h"

namespace vkgs {
namespace core {

ScreenSplatsImpl::ScreenSplatsImpl() {
  visible_point_count_ = gpu::Buffer::Create(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      sizeof(uint32_t));
  draw_indirect_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                       sizeof(VkDrawIndexedIndirectCommand));
  stats_ = gpu::Buffer::Create(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      sizeof(Stats));
}

ScreenSplatsImpl::~ScreenSplatsImpl() = default;

void ScreenSplatsImpl::Update(uint32_t point_count) {
  if (point_count_ < point_count) {
    instances_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 12 * sizeof(float));
    point_count_ = point_count;
  }
}

}  // namespace core
}  // namespace vkgs
