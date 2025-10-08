#include "vkgs/core/module.h"

#include "vkgs/core/gaussian_splats.h"

#include "device.h"
#include "sorter.h"
#include "buffer.h"

namespace vkgs {
namespace core {

Module::Module() {
  device_ = std::make_shared<Device>();
  sorter_ = std::make_shared<Sorter>(device_->device(), device_->physical_device(), device_->allocator());
}

const std::string& Module::device_name() const noexcept { return device_->device_name(); }
uint32_t Module::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Module::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Module::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Module::load_from_ply(const std::string& path) {
  // TODO: Implement
  size_t size = 0;
  std::shared_ptr<Buffer> position;
  std::shared_ptr<Buffer> cov3d;
  std::shared_ptr<Buffer> sh;
  std::shared_ptr<Buffer> opacity;
  return std::make_shared<GaussianSplats>(size, position, cov3d, sh, opacity);
}

}  // namespace core
}  // namespace vkgs
