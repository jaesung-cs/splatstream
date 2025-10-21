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

  // TODO: add a UNORM image for 32_SFLOAT -> 8_UNORM

  void Update(uint32_t width, uint32_t height);

 private:
  std::shared_ptr<gpu::Device> device_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TRANSFER_STORAGE_H
