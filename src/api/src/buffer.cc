#include "vkgs/buffer.h"

#include "vkgs/module.h"

#include "buffer_impl.h"

namespace vkgs {

Buffer::Buffer(Module& module, size_t size) : impl_(std::make_shared<Impl>(module, size)) {}

Buffer::~Buffer() = default;

size_t Buffer::size() const noexcept { return impl_->size(); }

void Buffer::ToGpu(void* ptr, size_t size) { impl_->ToGpu(ptr, size); }

}  // namespace vkgs
