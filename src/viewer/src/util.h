#ifndef VKGS_VIEWER_UTIL_H
#define VKGS_VIEWER_UTIL_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "pose.h"

namespace vkgs {
namespace viewer {

float SmoothDamp(float current, float target, float& current_velocity, float smooth_time, float delta_time);

glm::vec3 SmoothDamp(const glm::vec3& current, const glm::vec3& target, glm::vec3& current_velocity, float smooth_time,
                     float delta_time);

glm::quat SmoothDamp(const glm::quat& current, const glm::quat& target, glm::quat& current_velocity, float smooth_time,
                     float delta_time);

Pose SmoothDamp(const Pose& current, const Pose& target, Pose& current_velocity, float smooth_time, float delta_time);

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_UTIL_H
