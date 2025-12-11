#ifndef VKGS_CORE_STATS_H
#define VKGS_CORE_STATS_H

#include <cstdint>

namespace vkgs {
namespace core {

struct Stats {
  uint32_t histogram_alpha[64];
  uint32_t histogram_projection_active_threads[64];  // array index i means i+1 threads are active
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_STATS_H
