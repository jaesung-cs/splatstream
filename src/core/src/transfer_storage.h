#ifndef VKGS_CORE_TRANSFER_STORAGE_H
#define VKGS_CORE_TRANSFER_STORAGE_H

#include <memory>
#include <cstdint>

namespace vkgs {
namespace gpu {

class Device;
class Buffer;

}  // namespace gpu

namespace core {

class TransferStorage {
 public:
  TransferStorage(std::shared_ptr<gpu::Device> device);
  ~TransferStorage();

  auto image_buffer() const noexcept { return image_buffer_; }

  void Update(uint32_t width, uint32_t height);

 private:
  std::shared_ptr<gpu::Device> device_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
  std::shared_ptr<gpu::Buffer> image_buffer_;  // (H, W, 4) float32
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TRANSFER_STORAGE_H
