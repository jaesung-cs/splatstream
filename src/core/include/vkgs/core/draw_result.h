#ifndef VKGS_CORE_DRAW_RESULT_H
#define VKGS_CORE_DRAW_RESULT_H

#include <cstdint>

namespace vkgs {
namespace core {

struct DrawResult {
  uint64_t compute_timestamp;
  uint64_t graphics_timestamp;
  uint64_t transfer_timestamp;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DRAW_RESULT_H
