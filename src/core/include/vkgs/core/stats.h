#ifndef VKGS_CORE_STATS_H
#define VKGS_CORE_STATS_H

#include <cstdint>

namespace vkgs {
namespace core {

struct Stats {
  uint32_t histogram_alpha[50];
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_STATS_H
