#include "transfer_storage.h"

#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

TransferStorage::TransferStorage(std::shared_ptr<gpu::Device> device) : device_(device) {}

TransferStorage::~TransferStorage() {}

void TransferStorage::Update(uint32_t width, uint32_t height) {
  if (width_ != width || height_ != height) {
    width_ = width;
    height_ = height;
  }
}

}  // namespace core
}  // namespace vkgs