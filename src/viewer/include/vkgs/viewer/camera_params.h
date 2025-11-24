#ifndef VKGS_VIEWER_CAMERA_PARAMS_H
#define VKGS_VIEWER_CAMERA_PARAMS_H

#include <glm/glm.hpp>

namespace vkgs {
namespace viewer {

struct CameraParams {
  /**
   * W2C = [R T; 0 1]
   * x right, y down, z forward.
   */
  glm::mat4 extrinsic;

  /**
   * K = [fx 0 cx; 0 fy cy; 0 0 1]
   * u: [0, width], left to right.
   * v: [0, height], top to bottom.
   */
  glm::mat3 intrinsic;

  uint32_t width;
  uint32_t height;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_CAMERA_PARAMS_H
