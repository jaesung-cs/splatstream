#include "camera.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "util.h"

namespace vkgs {
namespace viewer {

Camera::Camera() {}

Camera::~Camera() {}

void Camera::SetWindowSize(uint32_t width, uint32_t height) {
  width_ = width;
  height_ = height;
}

void Camera::SetFov(float fov) {
  // dolly zoom
  float r0 = target_r_;
  target_r_ *= std::tan(target_fovy_ / 2.f) / std::tan(fov / 2.f);
  target_.p = target_.p - (r0 - target_r_) * glm::toMat3(target_.q)[2];

  target_fovy_ = fov;
}

void Camera::SetView(const glm::mat4& view) {
  // Decompose view matrix to quaternion and eye
  glm::mat4 cam = glm::inverse(view);
  target_.q = glm::quat_cast(cam);
  target_.p = cam[3];
}

void Camera::Update(float dt) {
  pose_ = SmoothDamp(pose_, target_, velocity_, 0.05f, dt);
  fovy_ = SmoothDamp(fovy_, target_fovy_, velocity_fovy_, 0.05f, dt);
  r_ = SmoothDamp(r_, target_r_, velocity_r_, 0.05f, dt);
}

glm::mat4 Camera::ProjectionMatrix() const {
  float aspect = static_cast<float>(width_) / height_;
  glm::mat4 projection = glm::perspective(fovy_, aspect, near_, far_);

  // gl to vulkan projection matrix
  glm::mat4 conversion = glm::mat4(1.f);
  conversion[1][1] = -1.f;
  conversion[2][2] = 0.5f;
  conversion[3][2] = 0.5f;
  return conversion * projection;
}

glm::mat4 Camera::ViewMatrix() const {
  auto cam = glm::toMat4(pose_.q);
  cam[3] = glm::vec4(pose_.p, 1.f);
  return glm::inverse(cam);
}

glm::vec3 Camera::Eye() const { return pose_.p; }

void Camera::Rotate(float dx, float dy) {
  if (dx == 0.f && dy == 0.f) return;
  dx = -dx;

  glm::vec3 axis = glm::normalize(glm::vec3(-dy, dx, 0.f));
  float angle = glm::length(glm::vec2(-dy, dx)) * rotation_sensitivity_;
  float c = std::cos(angle / 2.f);
  float s = std::sin(angle / 2.f);
  glm::quat dq = glm::quat(c, s * axis.x, s * axis.y, s * axis.z);
  target_.q = target_.q * dq;
}

void Camera::Translate(float dx, float dy, float dz) {
  auto rot = glm::toMat3(target_.q);
  target_.p += translation_sensitivity_ * r_ * (-dx * rot[0] + dy * rot[1] + -dz * rot[2]);
}

void Camera::Zoom(float x) {
  float r0 = target_r_;
  target_r_ /= std::exp(zoom_sensitivity_ * x);
  target_.p = target_.p - (r0 - target_r_) * glm::toMat3(target_.q)[2];
}

void Camera::DollyZoom(float scroll) {
  float new_fov = std::clamp(target_fovy_ - scroll * dolly_zoom_sensitivity_, min_fov(), max_fov());
  SetFov(new_fov);
}

}  // namespace viewer
}  // namespace vkgs
