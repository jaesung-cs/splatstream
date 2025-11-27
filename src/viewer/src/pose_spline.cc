#include "pose_spline.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace vkgs {
namespace viewer {
namespace {

glm::mat3 Skew(const glm::vec3& w) { return glm::mat3(0, -w.z, w.y, w.z, 0, -w.x, -w.y, w.x, 0); }

// Exp map: se(3) -> SE(3)
Pose Exp(const glm::mat2x3& xi) {
  glm::vec3 w = xi[0];
  glm::vec3 v = xi[1];

  float theta = glm::length(w);

  glm::quat q;
  glm::mat3 R{1.f};

  if (theta < 1e-10) {
    q = glm::quat{1.f, 0.f, 0.f, 0.f};
    R = glm::mat3{1.f};
  } else {
    glm::vec3 axis = w / theta;
    q = glm::angleAxis(theta, axis);
    R = glm::toMat3(q);
  }

  glm::mat3 V{1.f};

  if (theta > 1e-10) {
    float s = std::sin(theta);
    float c = std::cos(theta);

    glm::mat3 wx = Skew(w);
    V = glm::mat3{1.f} + (1.f - c) / (theta * theta) * wx + (theta - s) / (theta * theta * theta) * (wx * wx);
  }

  Pose out;
  out.q = q;
  out.p = V * v;
  return out;
}

// Log map: SE(3) -> se(3)
glm::mat2x3 Log(const Pose& pose) {
  glm::mat2x3 xi{0.f};

  float angle = 2.f * std::acos(glm::clamp(pose.q.w, -1.f, 1.f));

  glm::vec3 axis;
  if (angle < 1e-10) {
    axis = glm::vec3{0.f};
  } else {
    float s = std::sqrt(1 - pose.q.w * pose.q.w);
    axis = glm::vec3{pose.q.x, pose.q.y, pose.q.z} / s;
  }

  glm::vec3 w = angle * axis;

  glm::vec3 v = pose.p;
  glm::mat3 V_inv{1.f};

  if (angle > 1e-10) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    glm::mat3 wx = Skew(w);

    V_inv = glm::mat3{1.f} - 0.5f * wx + (1.f / (angle * angle) - (1.f + c) / (2.f * angle * s)) * (wx * wx);
  }

  xi[0] = w;

  glm::vec3 t = V_inv * v;
  xi[1] = t;

  return xi;
}

Pose Inverse(const Pose& p) {
  Pose r;
  r.q = glm::conjugate(p.q);
  r.p = glm::rotate(r.q, -p.p);
  return r;
}

// Compose SE(3) transforms
Pose Compose(const Pose& a, const Pose& b) {
  Pose r;
  r.q = a.q * b.q;
  r.p = glm::rotate(a.q, b.p) + a.p;
  return r;
}

}  // namespace

// Interpolate within one segment (Hermite-style C1)
Pose PoseSpline::InterpolateSegment(int i, float u) const {
  Pose P0 = poses_[(i + poses_.size() - 1) % poses_.size()];
  Pose P1 = poses_[i];
  Pose P2 = poses_[(i + 1) % poses_.size()];
  Pose P3 = poses_[(i + 2) % poses_.size()];

  if (glm::dot(P0.q, P1.q) < 0.f) P1.q = -P1.q;
  if (glm::dot(P1.q, P2.q) < 0.f) P2.q = -P2.q;
  if (glm::dot(P2.q, P3.q) < 0.f) P3.q = -P3.q;

  auto d01 = Log(Compose(Inverse(P0), P1));
  auto d12 = Log(Compose(Inverse(P1), P2));
  auto d23 = Log(Compose(Inverse(P2), P3));

  auto m1 = 0.5f * (d01 + d12);
  auto m2 = 0.5f * (d12 + d23);

  float u2 = u * u;
  float u3 = u2 * u;
  float h0 = 2.f * u3 - 3.f * u2 + 1.f;  // p0
  float h1 = u3 - 2.f * u2 + u;          // v0
  float h2 = -2.f * u3 + 3.f * u2;       // p1
  float h3 = u3 - u2;                    // v1

  auto xi = h1 * m1 + h2 * d12 + h3 * m2;
  return Compose(P1, Exp(xi));
}

// Evaluate spline at t in [0, N-1]
Pose PoseSpline::Evaluate(float t) const {
  t = std::fmod(t, poses_.size());
  int i = int(t);

  float u = t - i;
  return InterpolateSegment(i, u);
}

}  // namespace viewer
}  // namespace vkgs
