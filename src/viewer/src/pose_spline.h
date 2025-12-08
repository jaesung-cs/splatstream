#ifndef VKGS_VIEWER_POSE_SPLINE_H
#define VKGS_VIEWER_POSE_SPLINE_H

#include <vector>
#include <cmath>

#include "pose.h"

namespace vkgs {
namespace viewer {

class PoseSpline {
 public:
  PoseSpline() = default;
  PoseSpline(std::vector<Pose> poses) : poses_(std::move(poses)) {}

  void push_back(const Pose& pose) { poses_.push_back(pose); }
  void clear() { poses_.clear(); }

  // Evaluate spline at t in [0, N-1]
  Pose Evaluate(float t) const;

 private:
  // Interpolate within one segment (Hermite-style C1)
  Pose InterpolateSegment(int i, float u) const;

  std::vector<Pose> poses_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_POSE_SPLINE_H
