module;
#include <cmath>
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

module avalon.core;
import :math.quaternion;

namespace avalon {

static Quaternion FromGlmInternal(const glm::quat &gq) {
  return Quaternion(gq.x, gq.y, gq.z, gq.w);
}

Quaternion Quaternion::FromAxisAngle(const Vec3 &axis, float angleRadians) {
  float halfAngle = angleRadians * 0.5f;
  float s = std::sin(halfAngle);
  return Quaternion(axis.x * s, axis.y * s, axis.z * s, std::cos(halfAngle));
}

Quaternion Quaternion::FromEuler(const Vec3 &euler) {
  float ex = euler.x * kDeg2Rad * 0.5f;
  float ey = euler.y * kDeg2Rad * 0.5f;
  float ez = euler.z * kDeg2Rad * 0.5f;

  float cx = std::cos(ex);
  float sx = std::sin(ex);
  float cy = std::cos(ey);
  float sy = std::sin(ey);
  float cz = std::cos(ez);
  float sz = std::sin(ez);

  return Quaternion(sx * cy * cz + cx * sy * sz, cx * sy * cz - sx * cy * sz,
                    cx * cy * sz - sx * sy * cz, cx * cy * cz + sx * sy * sz);
}

Quaternion Quaternion::FromMatrix(const Matrix4x4 &m) {
  const auto &gm = *reinterpret_cast<const glm::mat4 *>(m.data);
  glm::quat gq = glm::quat_cast(gm);
  return Quaternion(gq.x, gq.y, gq.z, gq.w);
}

Quaternion Quaternion::LookRotation(const Vec3 &forward, const Vec3 &up) {
  glm::vec3 gf(forward.x, forward.y, forward.z);
  glm::vec3 gu(up.x, up.y, up.z);

  glm::vec3 nForward = glm::normalize(gf);
  glm::vec3 nUp = glm::normalize(gu);
  if (std::abs(glm::dot(nForward, nUp)) > 0.9999f) {
    return Quaternion::FromAxisAngle(Vec3(1, 0, 0), 0);
  }

  return FromGlmInternal(glm::quatLookAt(nForward, nUp));
}

Vec3 Quaternion::ToEuler() const {
  Vec3 euler;
  float sinp = 2.0f * (w * x - y * z);
  if (std::abs(sinp) >= 1.0f)
    euler.x = std::copysign(kHalfPi, sinp);
  else
    euler.x = std::asin(sinp);

  euler.y = std::atan2(2.0f * (w * y + z * x), 1.0f - 2.0f * (x * x + y * y));
  euler.z = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (z * z + x * x));

  return euler * kRad2Deg;
}

Quaternion Quaternion::operator*(const Quaternion &o) const {
  return Quaternion(w * o.x + x * o.w + y * o.z - z * o.y,
                    w * o.y - x * o.z + y * o.w + z * o.x,
                    w * o.z + x * o.y - y * o.x + z * o.w,
                    w * o.w - x * o.x - y * o.y - z * o.z);
}

void Quaternion::Normalize() {
  float magSq = x * x + y * y + z * z + w * w;
  if (std::abs(magSq - 1.0f) > kEpsilon) {
    float invMag = 1.f / std::sqrt(magSq);
    x *= invMag;
    y *= invMag;
    z *= invMag;
    w *= invMag;
  }
}

Vec3 Quaternion::Rotate(const Vec3 &v) const {
  Vec3 qv(x, y, z);
  Vec3 t = Vec3::Cross(qv, v) * 2.f;
  return v + t * w + Vec3::Cross(qv, t);
}

Matrix4x4 Quaternion::ToMatrix() const {
  glm::mat4 gm = glm::mat4_cast(glm::quat(w, x, y, z));
  Matrix4x4 res;
  *reinterpret_cast<glm::mat4 *>(res.data) = gm;
  return res;
}

String Quaternion::ToString() const {
  return String::Format("x: {:.4f}, y: {:.4f}, z: {:.4f}, w: {:.4f}", x, y, z,
                        w);
}

} // namespace avalon
