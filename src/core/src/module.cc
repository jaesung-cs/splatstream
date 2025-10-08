#include "vkgs/core/module.h"

#include "device.h"
#include "sorter.h"

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

}  // namespace core
}  // namespace vkgs
