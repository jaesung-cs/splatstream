#include "vkgs/core/screen_splats.h"

#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

ScreenSplats::ScreenSplats() {
  draw_indirect_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                       sizeof(VkDrawIndexedIndirectCommand));
}

ScreenSplats::~ScreenSplats() = default;

void ScreenSplats::Update(uint32_t point_count) {
  if (point_count_ < point_count) {
    instances_ = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 12 * sizeof(float));
    point_count_ = point_count;
  }
}

}  // namespace core
}  // namespace vkgs
