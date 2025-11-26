#ifndef VKGS_VIEWER_POSE_H
#define VKGS_VIEWER_POSE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vkgs {
namespace viewer {

struct Pose {
  glm::vec3 p;
  glm::quat q;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_POSE_H
