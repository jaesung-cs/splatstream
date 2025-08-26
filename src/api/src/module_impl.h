#ifndef VKGS_MODULE_IMPL_H
#define VKGS_MODULE_IMPL_H

#include <memory>
#include <string>

#include "vkgs/core/module.h"

#include "vkgs/module.h"

namespace vkgs {

class Module::Impl {
 public:
  Impl();

  ~Impl();

  const std::string& device_name() const noexcept { return module_->device_name(); }
  uint32_t graphics_queue_index() const noexcept { return module_->graphics_queue_index(); }
  uint32_t compute_queue_index() const noexcept { return module_->compute_queue_index(); }
  uint32_t transfer_queue_index() const noexcept { return module_->transfer_queue_index(); }

  void WaitIdle() { module_->WaitIdle(); }

  auto module() const noexcept { return module_; }

 private:
  std::shared_ptr<core::Module> module_;
};

}  // namespace vkgs

#endif  // VKGS_MODULE_IMPL_H
