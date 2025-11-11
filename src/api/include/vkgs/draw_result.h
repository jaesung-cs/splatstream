#ifndef VKGS_DRAW_RESULT_H
#define VKGS_DRAW_RESULT_H

#include <cstdint>

namespace vkgs {

struct DrawResult {
  uint64_t compute_timestamp;
  uint64_t graphics_timestamp;
  uint64_t transfer_timestamp;
};

}  // namespace vkgs

#endif  // VKGS_DRAW_RESULT_H
