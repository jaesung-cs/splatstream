#include "transfer_storage.h"

#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

TransferStorage::TransferStorage(std::shared_ptr<gpu::Device> device) : device_(device) {}

TransferStorage::~TransferStorage() {}

void TransferStorage::Update(uint32_t width, uint32_t height) {
  if (width_ != width || height_ != height) {
    image_buffer_ =
        gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, width * height * 4 * sizeof(float), true);

    width_ = width;
    height_ = height;
  }
}

}  // namespace core
}  // namespace vkgs