#ifndef VKGS_CORE_TRANSFER_STORAGE_H
#define VKGS_CORE_TRANSFER_STORAGE_H

#include <memory>
#include <cstdint>

namespace vkgs {
namespace gpu {

class Device;

}  // namespace gpu

namespace core {

class TransferStorage {
 public:
  TransferStorage();
  ~TransferStorage();

  void Update(uint32_t width, uint32_t height);

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TRANSFER_STORAGE_H
