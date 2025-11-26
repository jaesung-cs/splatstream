#include "util.h"

#include <algorithm>

namespace vkgs {
namespace viewer {

float SmoothDamp(float current, float target, float& current_velocity, float smooth_time, float delta_time) {
  smooth_time = std::max(0.0001f, smooth_time);
  float omega = 2.f / smooth_time;

  float x = omega * delta_time;
  float exp = std::exp(-x);
  float change = current - target;
  float original_to = target;
  target = current - change;

  float temp = (current_velocity + omega * change) * delta_time;
  current_velocity = (current_velocity - omega * temp) * exp;
  float output = target + (change + temp) * exp;

  // Prevent overshooting
  if ((original_to - current > 0.0F) == (output > original_to)) {
    output = original_to;
    current_velocity = (output - original_to) / delta_time;
  }

  return output;
}

glm::vec3 SmoothDamp(const glm::vec3& current, const glm::vec3& target, glm::vec3& current_velocity, float smooth_time,
                     float delta_time) {
  glm::vec3 result;
  result.x = SmoothDamp(current.x, target.x, current_velocity.x, smooth_time, delta_time);
  result.y = SmoothDamp(current.y, target.y, current_velocity.y, smooth_time, delta_time);
  result.z = SmoothDamp(current.z, target.z, current_velocity.z, smooth_time, delta_time);
  return result;
}

glm::quat SmoothDamp(const glm::quat& current, const glm::quat& target, glm::quat& current_velocity, float smooth_time,
                     float delta_time) {
  // https://gist.github.com/maxattack/4c7b4de00f5c1b95a33b
  float dot = glm::dot(current, target);
  float multi = dot > 0.f ? 1.f : -1.f;
  glm::quat near_target = target * multi;

  // smooth damp (nlerf approx)
  auto result =
      glm::normalize(glm::quat(SmoothDamp(current.w, near_target.w, current_velocity.w, smooth_time, delta_time),
                               SmoothDamp(current.x, near_target.x, current_velocity.x, smooth_time, delta_time),
                               SmoothDamp(current.y, near_target.y, current_velocity.y, smooth_time, delta_time),
                               SmoothDamp(current.z, near_target.z, current_velocity.z, smooth_time, delta_time)));

  // ensure velocity is tangent
  auto error = glm::dot(current_velocity, result) * result;
  current_velocity -= error;

  return result;
}

Pose SmoothDamp(const Pose& current, const Pose& target, Pose& current_velocity, float smooth_time, float delta_time) {
  Pose result;
  result.p = SmoothDamp(current.p, target.p, current_velocity.p, smooth_time, delta_time);
  result.q = SmoothDamp(current.q, target.q, current_velocity.q, smooth_time, delta_time);
  return result;
}

}  // namespace viewer
}  // namespace vkgs
