module;
#include <cstdint>

export module avalon.core:math.matrix;

import :string;
import :math.vector;

export namespace avalon {

struct alignas(16) Matrix4x4 {
  float data[4][4];

  constexpr Matrix4x4() : data{} {}
  constexpr Matrix4x4(Vec4 col0, Vec4 col1, Vec4 col2, Vec4 col3)
      : data{{col0.x, col0.y, col0.z, col0.w},
             {col1.x, col1.y, col1.z, col1.w},
             {col2.x, col2.y, col2.z, col2.w},
             {col3.x, col3.y, col3.z, col3.w}} {}

  constexpr Matrix4x4(float m00, float m01, float m02, float m03, float m10,
                      float m11, float m12, float m13, float m20, float m21,
                      float m22, float m23, float m30, float m31, float m32,
                      float m33)
      : data{{m00, m01, m02, m03},
             {m10, m11, m12, m13},
             {m20, m21, m22, m23},
             {m30, m31, m32, m33}} {}

  static const Matrix4x4 Identity;

  float *operator[](uint32_t index) { return data[index]; }
  const float *operator[](uint32_t index) const { return data[index]; }

  Vec3 GetRight() const;
  Vec3 GetLeft() const;
  Vec3 GetUp() const;
  Vec3 GetDown() const;
  Vec3 GetForward() const;
  Vec3 GetBack() const;

  void GetBasis(Vec3 &outRight, Vec3 &outUp, Vec3 &outForward) const;
  Vec3 GetTranslation() const;
  Vec3 GetColumn(uint32_t col) const;

  Vec4 operator*(const Vec4 &v) const;
  Matrix4x4 operator*(const Matrix4x4 &b) const;
  Matrix4x4 Transpose() const;
  Matrix4x4 Inverse() const;

  String ToString() const;
};

} // namespace avalon
