#include "buffer_impl.h"

#include "module_impl.h"

namespace vkgs {

Buffer::Impl::Impl(Module& module, size_t size) {
  buffer_ = std::make_shared<core::Buffer>(module.impl()->module(), size);
}

Buffer::Impl::~Impl() {}

}  // namespace vkgs
