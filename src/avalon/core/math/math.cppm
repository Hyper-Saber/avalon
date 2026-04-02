module;
#include <cmath>
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.core:math;

import :debug;
import :string;
import :log;
import :math.vector;
import :math.matrix;

export namespace avalon {
constexpr float kPi = 3.1415926535897932384626433832795f;
constexpr float kHalfPi = kPi * 0.5f;
constexpr float kTwoPi = kPi * 2.0f;
constexpr float kEpsilon = 0.000001f;
constexpr float kRad2Deg = 57.295779513082320876798154814105f;
constexpr float kDeg2Rad = 0.01745329251994329576923690768489f;

template <typename T> constexpr T Clamp(T value, T min, T max) {
  return value < min ? min : value > max ? max : value;
}

template <typename T> constexpr T Max(T a, T b) { return a > b ? a : b; }

template <typename T> constexpr T Min(T a, T b) { return a < b ? a : b; }

template <typename T> constexpr T Lerp(T a, T b, float t) {
  return a + (b - a) * t;
}

template <typename T> constexpr T Clamp01(T value) {
  return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}

template <typename T> constexpr T Abs(T value) {
  return value < 0 ? -value : value;
}

template <typename T> constexpr T Sign(T value) {
  return value < 0 ? -1 : value > 0 ? 1 : 0;
}

template <typename T> constexpr T Sqrt(T value) { return std::sqrt(value); }

template <typename T> constexpr T Pow(T base, T exponent) {
  return std::pow(base, exponent);
}

template <typename T> constexpr T Step(T edge, T value) {
  return value < edge ? 0.0f : 1.0f;
}

template <typename T> constexpr T SmoothStep(T edge0, T edge1, T value) {
  T t = Clamp01((value - edge0) / (edge1 - edge0));
  return t * t * (3 - 2 * t);
}

inline float Cos(float radians) { return std::cos(radians); }

inline float Sin(float radians) { return std::sin(radians); }

inline float Tan(float radians) { return std::tan(radians); }

inline float Acos(float value) { return std::acos(value); }

inline float Asin(float value) { return std::asin(value); }

inline float Atan(float value) { return std::atan(value); }

inline float Atan2(float y, float x) { return std::atan2(y, x); }

inline constexpr float ToRadians(float degrees) { return degrees * kDeg2Rad; }

inline constexpr float ToDegrees(float radians) { return radians * kRad2Deg; }

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

  return {0.0f, 0.0f, 0.0f};
}

inline float LengthSquared(const Vec2 &v) { return v.x * v.x + v.y * v.y; }

inline float Length(const Vec2 &v) { return std::sqrt(LengthSquared(v)); }

inline Vec2 Normalize(const Vec2 &v) {
  float lenSq = LengthSquared(v);

  if (lenSq > kEpsilon) {
    float invLen = 1.0f / std::sqrt(lenSq);
    return {v.x * invLen, v.y * invLen};
  }

  return {0.0f, 0.0f};
}

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
                                            float near) {
  float const fov = ToRadians(fovDeg);
  float h = 1.0f / std::tan(fov / 2.0f);
  float w = h / aspect;
  Matrix4x4 res;
  res.data[0][0] = w;
  res.data[1][1] = -h;
  res.data[2][2] = 0;
  res.data[2][3] = -1.0f;
  res.data[3][2] = near;
  res.data[3][3] = 0;
  return res;
}

inline Matrix4x4 CalculateInversePerspective(float fovDeg, float aspect,
                                             float near) {
  float const fov = ToRadians(fovDeg);
  float h = 1.0f / std::tan(fov / 2.0f);
  float w = h / aspect;

  Matrix4x4 res;
  res.data[0][0] = 1.0f / w;
  res.data[1][1] = 1.0f / (-h);

  res.data[2][2] = 0.0f;
  res.data[3][2] = -1.0f;
  res.data[2][3] = 1.0f / near;
  res.data[3][3] = 0.0f;

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

inline auto ComputeNormalMatrix(const Matrix4x4 &model) {
  return model.Transpose().Inverse();
}

} // namespace avalon
