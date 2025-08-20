#ifndef VKGS_BUFFER_H
#define VKGS_BUFFER_H

#include "vkgs/export_api.h"
#include "vkgs/module.h"

namespace vkgs {

class VKGS_API Buffer {
 public:
  Buffer(Module* module, size_t size);
  ~Buffer();

  size_t size() const noexcept;

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_BUFFER_H
