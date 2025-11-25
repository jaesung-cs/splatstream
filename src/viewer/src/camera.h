#ifndef VKGS_VIEWER_CAMERA_H
#define VKGS_VIEWER_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vkgs {
namespace viewer {

class Camera {
 public:
  static constexpr float min_fov() { return glm::radians(40.f); }
  static constexpr float max_fov() { return glm::radians(100.f); }

 public:
  Camera();
  ~Camera();

  auto Near() const noexcept { return near_; }
  auto Far() const noexcept { return far_; }

  void SetWindowSize(uint32_t width, uint32_t height);

  /**
   * Set fov and dolly zoom
   *
   * fov: fov Y, in radians
   */
  void SetFov(float fov);

  void SetView(const glm::mat4& view);

  glm::mat4 ProjectionMatrix() const;
  glm::mat4 ViewMatrix() const;
  glm::vec3 Eye() const;
  uint32_t width() const noexcept { return width_; }
  uint32_t height() const noexcept { return height_; }
  auto fov() const noexcept { return fovy_; }

  void Rotate(float dx, float dy);
  void Translate(float dx, float dy, float dz = 0.f);
  void Zoom(float dx);
  void DollyZoom(float scroll);

 private:
  uint32_t width_ = 256;
  uint32_t height_ = 256;
  float fovy_ = glm::radians(60.f);
  float near_ = 0.01f;
  float far_ = 100.f;

  glm::vec3 center_ = {0.f, 0.f, 0.f};
  // camera = center + r * Rot(quat).z
  float r_ = 2.f;
  glm::quat quat_ = {1.f, 0.f, 0.f, 0.f};

  float rotation_sensitivity_ = 0.01f;
  float translation_sensitivity_ = 0.002f;
  float zoom_sensitivity_ = 0.01f;
  float dolly_zoom_sensitivity_ = glm::radians(1.f);
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_CAMERA_H
