#ifndef VKGS_CORE_DRAW_OPTIONS_H
#define VKGS_CORE_DRAW_OPTIONS_H

#include <glm/glm.hpp>

namespace vkgs {
namespace core {

struct DrawOptions {
  glm::mat4 view;
  glm::mat4 projection;
  uint32_t width;
  uint32_t height;
  glm::vec3 background;
  float eps2d;
  int sh_degree;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DRAW_OPTIONS_H
