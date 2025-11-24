#ifndef VKGS_CAMERA_PARAMS_H
#define VKGS_CAMERA_PARAMS_H

#include <cstdint>

namespace vkgs {

struct CameraParams {
  /**
   * 4x4 column-major.
   * W2C = [R T; 0 1]
   * x right, y down, z forward.
   */
  float extrinsic[16];

  /**
   * 3x3 column-major.
   * K = [fx 0 cx; 0 fy cy; 0 0 1]
   * u: [0, width], left to right.
   * v: [0, height], top to bottom.
   */
  float intrinsic[9];

  uint32_t width;
  uint32_t height;
};

}  // namespace vkgs

#endif  // VKGS_CAMERA_PARAMS_H
