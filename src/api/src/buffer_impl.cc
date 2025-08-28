#include "buffer_impl.h"

#include "module_impl.h"

namespace vkgs {

Buffer::Impl::Impl(Module& module, size_t size) {
  buffer_ = std::make_shared<core::Buffer>(module.impl()->module(), size);
}

Buffer::Impl::~Impl() {}

void Buffer::Impl::ToGpu(const void* ptr, size_t size) { buffer_->ToGpu(ptr, size); }

void Buffer::Impl::ToCpu(void* ptr, size_t size) { buffer_->ToCpu(ptr, size); }

void Buffer::Impl::Fill(uint32_t value) { buffer_->Fill(value); }

void Buffer::Impl::Sort() { buffer_->Sort(); }

}  // namespace vkgs
