module;
#include <cmath>
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.core:math;

import :debug;
import :string;
import :log;

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
  auto ToString() { return String::Format("x: {}, y: {}", x, y); }
};

struct Vec3 {
  float x = 0, y = 0, z = 0;

  Vec3 operator+(const Vec3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
  Vec3 operator-(const Vec3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
  Vec3 operator+(float s) const { return {x + s, y + s, z + s}; }
  Vec3 operator-(float s) const { return {x - s, y - s, z - s}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3 operator+=(float s) {
    x += s;
    y += s;
    z += s;
    return *this;
  }
  Vec3 operator-=(float s) {
    x -= s;
    y -= s;
    z -= s;
    return *this;
  }
  Vec3 operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  static Vec3 Right() { return {1.0f, 0.0f, 0.0f}; }
  static Vec3 Left() { return {-1.0f, 0.0f, 0.0f}; }
  static Vec3 Up() { return {0.0f, 1.0f, 0.0f}; }
  static Vec3 Down() { return {0.0f, -1.0f, 0.0f}; }
  static Vec3 Backward() { return {0.0f, 0.0f, 1.0f}; }
  static Vec3 Forward() { return {0.0f, 0.0f, -1.0f}; }

  static Vec3 Zero() { return {0.0f, 0.0f, 0.0f}; }
  static Vec3 One() { return {1.0f, 1.0f, 1.0f}; }

  auto ToString() { return String::Format("x: {}, y: {}, z: {}", x, y, z); }
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

  AVALON_ASSERT_MSG(false, "Attempting to normalize a zero-length vector!");

  return {0.0f, 0.0f, 0.0f};
}
struct Vec4 {
  float x = 0, y = 0, z = 0, w = 1;

  static Vec4 FromVec3(const Vec3 &v, float w = 1.0f) {
    return {v.x, v.y, v.z, w};
  }

  auto ToString() {
    return String::Format("x: {}, y: {}, z: {}, w: {}", x, y, z, w);
  }
};

struct alignas(16) Matrix4x4 {
  float data[4][4];

  constexpr Matrix4x4() : data{} {}

  constexpr Matrix4x4(float m00, float m01, float m02, float m03, float m10,
                      float m11, float m12, float m13, float m20, float m21,
                      float m22, float m23, float m30, float m31, float m32,
                      float m33)
      : data{{m00, m01, m02, m03},
             {m10, m11, m12, m13},
             {m20, m21, m22, m23},
             {m30, m31, m32, m33}} {}

  static const Matrix4x4 Identity;

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

  Matrix4x4 Transpose() const {
    Matrix4x4 res;
    res.data[0][0] = data[0][0];
    res.data[1][1] = data[1][1];
    res.data[2][2] = data[2][2];
    res.data[3][3] = data[3][3];

    res.data[0][1] = data[1][0];
    res.data[1][0] = data[0][1];

    res.data[0][2] = data[2][0];
    res.data[2][0] = data[0][2];

    res.data[0][3] = data[3][0];
    res.data[3][0] = data[0][3];

    res.data[1][2] = data[2][1];
    res.data[2][1] = data[1][2];

    res.data[1][3] = data[3][1];
    res.data[3][1] = data[1][3];

    res.data[2][3] = data[3][2];
    res.data[3][2] = data[2][3];

    return res;
  }

  String ToString() const {
    return String::Format("\n[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]\n"
                          "[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]\n"
                          "[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]\n"
                          "[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]",
                          data[0][0], data[1][0], data[2][0], data[3][0],
                          data[0][1], data[1][1], data[2][1], data[3][1],
                          data[0][2], data[1][2], data[2][2], data[3][2],
                          data[0][3], data[1][3], data[2][3], data[3][3]);
  }
};

inline constexpr Matrix4x4 Matrix4x4::Identity =
    Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

inline Matrix4x4 Orthographic(float left, float right, float bottom, float top,
                              float zNear, float zFar) {
  Matrix4x4 res = Matrix4x4::Identity;
  res.data[0][0] = 2.0f / (right - left);
  res.data[1][1] = 2.0f / (bottom - top);
  res.data[2][2] = 1.0f / (zFar - zNear);
  res.data[3][0] = -(right + left) / (right - left);
  res.data[3][1] = -(bottom + top) / (bottom - top);
  res.data[3][2] = -zNear / (zFar - zNear);
  return res;
}

inline Matrix4x4 Translate(const Vec3 &v) {
  Matrix4x4 res = Matrix4x4::Identity;
  res.data[3][0] = v.x;
  res.data[3][1] = v.y;
  res.data[3][2] = v.z;

  return res;
}

inline Matrix4x4 Scale(const Vec3 &v) {
  Matrix4x4 res = Matrix4x4::Identity;
  res.data[0][0] = v.x;
  res.data[1][1] = v.y;
  res.data[2][2] = v.z;

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
inline Matrix4x4 Rotate(const Vec3 &rotation) {
  auto res = Matrix4x4::Identity;
  res = RotateY(res, rotation.y);
  res = RotateX(res, rotation.x);
  res = RotateZ(res, rotation.z);

  return res;
}

inline Matrix4x4 CalculatePerspectiveMatrix(float fovDeg, float aspect,
                                            float near, float far) {
  float const fov = ToRadians(fovDeg);
  float h = 1.0f / std::tan(fov / 2.0f);
  float w = h / aspect;
  Matrix4x4 res;
  res.data[0][0] = w;
  res.data[1][1] = -h;
  res.data[2][2] = far / (near - far);
  res.data[2][3] = -1.0f;
  res.data[3][2] = (far * near) / (near - far);
  res.data[3][3] = 0;
  return res;
}

inline Matrix4x4 CalculateViewMatrix(const Vec3 &position,
                                     const Vec3 &rotation) {
  auto res = Rotate(rotation).Transpose();

  res.data[3][0] = -(res.data[0][0] * position.x + res.data[1][0] * position.y +
                     res.data[2][0] * position.z);
  res.data[3][1] = -(res.data[0][1] * position.x + res.data[1][1] * position.y +
                     res.data[2][1] * position.z);
  res.data[3][2] = -(res.data[0][2] * position.x + res.data[1][2] * position.y +
                     res.data[2][2] * position.z);

  return res;
}

} // namespace avalon
