
#ifndef VKGS_CORE_STRUCT_H
#define VKGS_CORE_STRUCT_H

#include <cstdint>

#include <glm/glm.hpp>

namespace vkgs {
namespace core {

struct ParsePushConstants {
  alignas(16) uint32_t point_count;
  uint32_t sh_degree;
  uint32_t aligned_point_count;
};

struct ProjectionPushConstants {
  alignas(16) glm::mat4 model;
  uint32_t point_count;
  uint32_t aligned_point_count;
  uint32_t sh_degree_data;
  uint32_t sh_degree_draw;
  float eps2d;
};

struct Camera {
  alignas(16) glm::mat4 projection;
  glm::mat4 view;
  glm::vec4 camera_position;
  glm::uvec2 screen_size;
};

struct SplatPushConstants {
  alignas(16) glm::mat4 projection_inverse;
  float confidence_radius;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_STRUCT_H
