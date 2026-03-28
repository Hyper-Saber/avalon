module;
#include <cmath>
export module avalon.core:math.quaternion;

import :math;

export namespace avalon {
struct Quaternion {
  float x = 0, y = 0, z = 0, w = 1;

  Quaternion() {}
  Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

  static Quaternion FromAxisAngle(const Vec3 &axis, float angleRadians) {
    float halfAngle = angleRadians * 0.5f;
    float s = std::sin(halfAngle);
    return Quaternion(axis.x * s, axis.y * s, axis.z * s, std::cos(halfAngle));
  }

  static Quaternion LookRotation(const Vec3 &forward, const Vec3 &up);

  static Quaternion FromEuler(const Vec3 &euler) {
    float ex = euler.x * kDeg2Rad;
    float ey = euler.y * kDeg2Rad;
    float ez = euler.z * kDeg2Rad;

    float cx = std::cos(ex * 0.5f);
    float sx = std::sin(ex * 0.5f);
    float cy = std::cos(ey * 0.5f);
    float sy = std::sin(ey * 0.5f);
    float cz = std::cos(ez * 0.5f);
    float sz = std::sin(ez * 0.5f);

    return Quaternion(sx * cy * cz + cx * sy * sz, cx * sy * cz - sx * cy * sz,
                      cx * cy * sz - sx * sy * cz, cx * cy * cz + sx * sy * sz);
  }

  Vec3 ToEuler() const {
    Vec3 euler;
    float sinp = 2.0f * (w * x - y * z);
    if (std::abs(sinp) >= 1.0f)
      euler.x = std::copysign(kHalfPi, sinp);
    else
      euler.x = std::asin(sinp);

    float siny_cosp = 2.0f * (w * y + z * x);
    float cosy_cosp = 1.0f - 2.0f * (x * x + y * y);
    euler.y = std::atan2(siny_cosp, cosy_cosp);

    float sinr_cosp = 2.0f * (w * z + x * y);
    float cosr_cosp = 1.0f - 2.0f * (z * z + x * x);
    euler.z = std::atan2(sinr_cosp, cosr_cosp);

    return euler * kRad2Deg;
  }

  Quaternion operator*(const Quaternion &other) const {
    return Quaternion(w * other.x + x * other.w + y * other.z - z * other.y,
                      w * other.y - x * other.z + y * other.w + z * other.x,
                      w * other.z + x * other.y - y * other.x + z * other.w,
                      w * other.w - x * other.x - y * other.y - z * other.z);
  }

  void Normalize() {
    float magSq = x * x + y * y + z * z + w * w;
    if (std::abs(magSq - 1.0f) > kEpsilon) {
      float invMag = 1.f / std::sqrt(magSq);
      x *= invMag;
      y *= invMag;
      z *= invMag;
      w *= invMag;
    }
  }

  Vec3 Rotate(const Vec3 &v) const {
    // v' = q*v*q^-1
    Vec3 qv(x, y, z);
    Vec3 t = Vec3::Cross(qv, v) * 2.f;
    return v + t * w + Vec3::Cross(qv, t);
  }

  inline Matrix4x4 ToMatrix() const {
    Matrix4x4 res = Matrix4x4::Identity;

    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    res.data[0][0] = 1.0f - 2.0f * (yy + zz);
    res.data[0][1] = 2.0f * (xy + wz);
    res.data[0][2] = 2.0f * (xz - wy);
    res.data[0][3] = 0.0f;

    res.data[1][0] = 2.0f * (xy - wz);
    res.data[1][1] = 1.0f - 2.0f * (xx + zz);
    res.data[1][2] = 2.0f * (yz + wx);
    res.data[1][3] = 0.0f;

    res.data[2][0] = 2.0f * (xz + wy);
    res.data[2][1] = 2.0f * (yz - wx);
    res.data[2][2] = 1.0f - 2.0f * (xx + yy);
    res.data[2][3] = 0.0f;

    res.data[3][0] = 0.0f;
    res.data[3][1] = 0.0f;
    res.data[3][2] = 0.0f;
    res.data[3][3] = 1.0f;

    return res;
  }

  static Quaternion FromMatrix(const Matrix4x4 &m) {
    float trace = m.data[0][0] + m.data[1][1] + m.data[2][2];
    Quaternion q;

    if (trace > 0.0f) {
      float s = std::sqrt(trace + 1.0f) * 2.0f;
      q.w = 0.25f * s;
      q.x = (m.data[2][1] - m.data[1][2]) / s;
      q.y = (m.data[0][2] - m.data[2][0]) / s;
      q.z = (m.data[1][0] - m.data[0][1]) / s;
    } else if ((m.data[0][0] > m.data[1][1]) && (m.data[0][0] > m.data[2][2])) {
      float s =
          std::sqrt(1.0f + m.data[0][0] - m.data[1][1] - m.data[2][2]) * 2.0f;
      q.w = (m.data[2][1] - m.data[1][2]) / s;
      q.x = 0.25f * s;
      q.y = (m.data[1][0] + m.data[0][1]) / s;
      q.z = (m.data[2][0] + m.data[0][2]) / s;
    } else if (m.data[1][1] > m.data[2][2]) {
      float s =
          std::sqrt(1.0f + m.data[1][1] - m.data[0][0] - m.data[2][2]) * 2.0f;
      q.w = (m.data[0][2] - m.data[2][0]) / s;
      q.x = (m.data[1][0] + m.data[0][1]) / s;
      q.y = 0.25f * s;
      q.z = (m.data[2][1] + m.data[1][2]) / s;
    } else {
      float s =
          std::sqrt(1.0f + m.data[2][2] - m.data[0][0] - m.data[1][1]) * 2.0f;
      q.w = (m.data[1][0] - m.data[0][1]) / s;
      q.x = (m.data[2][0] + m.data[0][2]) / s;
      q.y = (m.data[2][1] + m.data[1][2]) / s;
      q.z = 0.25f * s;
    }

    q.Normalize();
    return q;
  }

  auto ToString() {
    return String::Format("x: {}, y: {}, z: {}, w: {}", x, y, z, w);
  }
};

} // namespace avalon
