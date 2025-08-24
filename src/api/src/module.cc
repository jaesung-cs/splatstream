#include "vkgs/module.h"

#include "module_impl.h"

namespace vkgs {

Module::Module() : impl_(std::make_shared<Impl>()) {}

Module::~Module() = default;

const std::string& Module::device_name() const noexcept { return impl_->device_name(); }
uint32_t Module::graphics_queue_index() const noexcept { return impl_->graphics_queue_index(); }
uint32_t Module::compute_queue_index() const noexcept { return impl_->compute_queue_index(); }
uint32_t Module::transfer_queue_index() const noexcept { return impl_->transfer_queue_index(); }

void Module::WriteBuffer(Buffer& buffer, void* ptr) { impl_->WriteBuffer(buffer, ptr); }
void Module::WaitIdle() { impl_->WaitIdle(); }

}  // namespace vkgs
