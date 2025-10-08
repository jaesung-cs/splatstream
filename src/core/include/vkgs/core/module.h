#ifndef VKGS_CORE_MODULE_H
#define VKGS_CORE_MODULE_H

#include <string>
#include <cstdint>
#include <memory>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class Device;
class Sorter;

class VKGS_CORE_API Module {
 public:
  Module();
  ~Module() = default;

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

 private:
  std::shared_ptr<Device> device_;
  std::shared_ptr<Sorter> sorter_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_MODULE_H
