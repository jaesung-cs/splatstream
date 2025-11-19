#include "transfer_storage.h"

namespace vkgs {
namespace core {

TransferStorage::TransferStorage() {}

TransferStorage::~TransferStorage() {}

void TransferStorage::Update(uint32_t width, uint32_t height) {
  if (width_ != width || height_ != height) {
    width_ = width;
    height_ = height;
  }
}

}  // namespace core
}  // namespace vkgs
