module;
#include <cmath>
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.core:math;

import :debug;

export namespace avalon {
constexpr float kPi = 3.1415926535897932384626433832795f;
constexpr float kEpsilon = 0.000001f;

inline constexpr float ToRadians(float degrees) {
  return degrees * (kPi / 180.f);
}

inline constexpr float ToDegrees(float radians) {
  return radians * (180.f / kPi);
}

struct Vec2 {
  float x = 0, y = 0;
};

struct Vec3 {
  float x = 0, y = 0, z = 0;

  Vec3 operator+(const Vec3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
  Vec3 operator-(const Vec3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }

  static Vec3 Right() { return {1.0f, 0.0f, 0.0f}; }
  static Vec3 Left() { return {-1.0f, 0.0f, 0.0f}; }
  static Vec3 Up() { return {0.0f, 1.0f, 0.0f}; }
  static Vec3 Down() { return {0.0f, -1.0f, 0.0f}; }
  static Vec3 Backward() { return {0.0f, 0.0f, 1.0f}; }
  static Vec3 Forward() { return {0.0f, 0.0f, -1.0f}; }

  static Vec3 Zero() { return {0.0f, 0.0f, 0.0f}; }
  static Vec3 One() { return {1.0f, 1.0f, 1.0f}; }
};

inline float LengthSquared(const Vec3 &v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float Length(const Vec3 &v) { return std::sqrt(LengthSquared(v)); }

inline Vec3 Normalize(const Vec3 &v) {
  float lenSq = LengthSquared(v);

  if (lenSq > kEpsilon) {
    float invLen = 1.0f / std::sqrt(lenSq);
    return {v.x * invLen, v.y * invLen, v.z * invLen};
  }

  AVALON_ASSERT_MSG(true, "Attempting to normalize a zero-length vector!");

  return {0.0f, 0.0f, 0.0f};
}
struct Vec4 {
  float x = 0, y = 0, z = 0, w = 1;

  static Vec4 FromVec3(const Vec3 &v, float w = 1.0f) {
    return {v.x, v.y, v.z, w};
  }
};

struct alignas(16) Matrix4x4 {
  float data[4][4];

  Matrix4x4() {
    for (uint32_t i = 0; i < 4; i++) {
      for (uint32_t j = 0; j < 4; j++) {
        data[i][j] = (i == j) ? 1.0f : 0.0f;
      }
    }
  }

  Matrix4x4 operator*(const Matrix4x4 &b) const {
    Matrix4x4 res;
    for (uint32_t col = 0; col < 4; col++) {
      for (uint32_t row = 0; row < 4; row++) {
        res.data[col][row] =
            data[0][row] * b.data[col][0] + data[1][row] * b.data[col][1] +
            data[2][row] * b.data[col][2] + data[3][row] * b.data[col][3];
      }
    }
    return res;
  }
};

inline Matrix4x4 Ortho(float left, float right, float bottom, float top,
                       float zNear, float zFar) {
  Matrix4x4 res;
  res.data[0][0] = 2.0f / (right - left);
  res.data[1][1] = 2.0f / (bottom - top);
  res.data[2][2] = 1.0f / (zFar - zNear);
  res.data[3][0] = -(right + left) / (right - left);
  res.data[3][1] = -(bottom + top) / (bottom - top);
  res.data[3][2] = -zNear / (zFar - zNear);
  return res;
}

inline Matrix4x4 Translate(const Matrix4x4 &m, const Vec3 &v) {
  Matrix4x4 res = m;
  res.data[3][0] = m.data[0][0] * v.x + m.data[1][0] * v.y +
                   m.data[2][0] * v.z + m.data[3][0];
  res.data[3][1] = m.data[0][1] * v.x + m.data[1][1] * v.y +
                   m.data[2][1] * v.z + m.data[3][1];
  res.data[3][2] = m.data[0][2] * v.x + m.data[1][2] * v.y +
                   m.data[2][2] * v.z + m.data[3][2];
  return res;
}

inline Matrix4x4 Scale(const Matrix4x4 &m, const Vec3 &v) {
  Matrix4x4 res = m;
  res.data[0][0] = m.data[0][0] * v.x;
  res.data[0][1] = m.data[0][1] * v.x;
  res.data[0][2] = m.data[0][2] * v.x;
  res.data[0][3] = m.data[0][3] * v.x;

  res.data[1][0] = m.data[1][0] * v.y;
  res.data[1][1] = m.data[1][1] * v.y;
  res.data[1][2] = m.data[1][2] * v.y;
  res.data[1][3] = m.data[1][3] * v.y;

  res.data[2][0] = m.data[2][0] * v.z;
  res.data[2][1] = m.data[2][1] * v.z;
  res.data[2][2] = m.data[2][2] * v.z;
  res.data[2][3] = m.data[2][3] * v.z;
  return res;
}

inline Matrix4x4 RotateX(const Matrix4x4 &m, float degrees) {
  auto radians = ToRadians(degrees);
  float c = std::cos(radians);
  float s = std::sin(radians);
  Matrix4x4 res = m;

  for (uint32_t i = 0; i < 4; i++) {
    float m1 = m.data[1][i];
    float m2 = m.data[2][i];
    res.data[1][i] = m1 * c + m2 * s;
    res.data[2][i] = m1 * -s + m2 * c;
  }
  return res;
}

inline Matrix4x4 RotateY(const Matrix4x4 &m, float degrees) {
  auto radians = ToRadians(degrees);
  float c = std::cos(radians);
  float s = std::sin(radians);
  Matrix4x4 res = m;

  for (uint32_t i = 0; i < 4; i++) {
    float m0 = m.data[0][i];
    float m2 = m.data[2][i];
    res.data[0][i] = m0 * c + m2 * -s;
    res.data[2][i] = m0 * s + m2 * c;
  }
  return res;
}

inline Matrix4x4 RotateZ(const Matrix4x4 &m, float degrees) {
  auto radians = ToRadians(degrees);
  float c = std::cos(radians);
  float s = std::sin(radians);
  Matrix4x4 res = m;

  for (uint32_t i = 0; i < 4; i++) {
    float m0 = m.data[0][i];
    float m1 = m.data[1][i];
    res.data[0][i] = m0 * c + m1 * s;
    res.data[1][i] = m0 * -s + m1 * c;
  }
  return res;
}

} // namespace avalon
