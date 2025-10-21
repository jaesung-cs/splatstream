#ifndef VKGS_CORE_TRANSFER_STORAGE_H
#define VKGS_CORE_TRANSFER_STORAGE_H

#include <memory>
#include <cstdint>

namespace vkgs {
namespace gpu {

class Device;
class Image;

}  // namespace gpu

namespace core {

class TransferStorage {
 public:
  TransferStorage(std::shared_ptr<gpu::Device> device);
  ~TransferStorage();

  std::shared_ptr<gpu::Image> image_u8() const noexcept { return image_u8_; }

  void Update(uint32_t width, uint32_t height);

 private:
  std::shared_ptr<gpu::Device> device_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
  std::shared_ptr<gpu::Image> image_u8_;  // (H, W, 4), UNORM
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TRANSFER_STORAGE_H
