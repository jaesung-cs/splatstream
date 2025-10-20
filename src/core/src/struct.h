
#ifndef VKGS_CORE_STRUCT_H
#define VKGS_CORE_STRUCT_H

#include <glm/glm.hpp>

namespace vkgs {
namespace core {

struct PushConstants {
  alignas(16) glm::mat4 model;
  alignas(16) uint32_t point_count;
};

struct Camera {
  alignas(16) glm::mat4 projection;
  alignas(16) glm::mat4 view;
  alignas(16) glm::vec4 camera_position;
  alignas(16) glm::uvec2 screen_size;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_STRUCT_H
