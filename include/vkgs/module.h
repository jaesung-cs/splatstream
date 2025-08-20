#ifndef VKGS_MODULE_H
#define VKGS_MODULE_H

#include <memory>
#include <string>

#include "vkgs/export_api.h"

namespace vkgs {

class Buffer;

class VKGS_API Module {
 public:
  Module();
  ~Module();

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  void write_buffer(Buffer& buffer, void* ptr);

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_MODULE_H