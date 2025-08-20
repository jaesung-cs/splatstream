#include "vkgs/buffer.h"

#include "vkgs/impl/buffer_impl.h"

namespace vkgs {

Buffer::Buffer(Module* module, size_t size) : impl_(std::make_unique<Impl>(module, size)) {}

Buffer::~Buffer() = default;

size_t Buffer::size() const noexcept { return impl_->size(); }

}  // namespace vkgs
