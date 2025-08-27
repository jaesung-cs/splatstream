#ifndef VKGS_BUFFER_H
#define VKGS_BUFFER_H

#include <memory>

#include "vkgs/export_api.h"

namespace vkgs {

class Module;

class VKGS_API Buffer {
 public:
  Buffer(Module& module, size_t size);
  ~Buffer();

  size_t size() const noexcept;

  void ToGpu(const void* ptr, size_t size);
  void ToCpu(void* ptr, size_t size);
  void Fill(uint32_t value);

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_BUFFER_H
