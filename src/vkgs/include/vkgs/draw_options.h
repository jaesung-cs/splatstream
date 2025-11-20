#ifndef VKGS_DRAW_OPTIONS_H
#define VKGS_DRAW_OPTIONS_H

#include <cstdint>

namespace vkgs {

struct DrawOptions {
  float view[16];        // column-major
  float projection[16];  // column-major
  float model[16];       // column-major
  uint32_t width;
  uint32_t height;
  float background[3];
  float eps2d;
  int sh_degree;
};

}  // namespace vkgs

#endif  // VKGS_DRAW_OPTIONS_H
