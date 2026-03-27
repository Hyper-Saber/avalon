module;
#include <cmath>
#include <cstdint>
export module avalon.core:math.matrix;

import :string;
import :math.vector;

export namespace avalon {

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

  float *operator[](uint32_t index) { return data[index]; }

  const float *operator[](uint32_t index) const { return data[index]; }

  Vec3 GetRight() const { return GetColumn(0); }
  Vec3 GetLeft() const { return -GetColumn(0); }
  Vec3 GetUp() const { return GetColumn(1); }
  Vec3 GetDown() const { return -GetColumn(1); }
  Vec3 GetForward() const { return -GetColumn(2); }
  Vec3 GetBack() const { return GetColumn(2); }

  void GetBasis(Vec3 &outRight, Vec3 &outUp, Vec3 &outForward) const {
    outRight = GetColumn(0);
    outUp = GetColumn(1);
    outForward = -GetColumn(2);
  }

  Vec3 GetTranslation() const {
    return Vec3{data[3][0], data[3][1], data[3][2]};
  }

  Vec3 GetColumn(uint32_t col) const {
    return Vec3{data[col][0], data[col][1], data[col][2]};
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

  Matrix4x4 Inverse() const {
    const float a = data[0][0], b = data[0][1], c = data[0][2], d = data[0][3];
    const float e = data[1][0], f = data[1][1], g = data[1][2], h = data[1][3];
    const float i = data[2][0], j = data[2][1], k = data[2][2], l = data[2][3];
    const float m = data[3][0], n = data[3][1], o = data[3][2], p = data[3][3];

    float v0 = k * p - l * o;
    float v1 = j * p - l * n;
    float v2 = j * o - k * n;
    float v3 = i * p - l * m;
    float v4 = i * o - k * m;
    float v5 = i * n - j * m;

    float d0 = (f * v0 - g * v1 + h * v2);
    float d1 = -(e * v0 - g * v3 + h * v4);
    float d2 = (e * v1 - f * v3 + h * v5);
    float d3 = -(e * v2 - f * v4 + g * v5);

    float det = a * d0 + b * d1 + c * d2 + d * d3;

    if (std::abs(det) < 1e-9f) [[unlikely]] {
      return Matrix4x4::Identity;
    }

    const float invDet = 1.0f / det;

    Matrix4x4 res;
    res.data[0][0] = d0 * invDet;
    res.data[1][0] = d1 * invDet;
    res.data[2][0] = d2 * invDet;
    res.data[3][0] = d3 * invDet;

    float v6 = g * p - h * o;
    float v7 = f * p - h * n;
    float v8 = f * o - g * n;
    float v9 = e * p - h * m;
    float v10 = e * o - g * m;
    float v11 = e * n - f * m;

    res.data[0][1] = -(b * v0 - c * v1 + d * v2) * invDet;
    res.data[1][1] = (a * v0 - c * v3 + d * v4) * invDet;
    res.data[2][1] = -(a * v1 - b * v3 + d * v5) * invDet;
    res.data[3][1] = (a * v2 - b * v4 + c * v5) * invDet;

    float v12 = c * h - d * g;
    float v13 = b * h - d * f;
    float v14 = b * g - c * f;
    float v15 = a * h - d * e;
    float v16 = a * g - c * e;
    float v17 = a * f - b * e;

    res.data[0][2] = (b * v6 - c * v7 + d * v8) * invDet;
    res.data[1][2] = -(a * v6 - c * v9 + d * v10) * invDet;
    res.data[2][2] = (a * v7 - b * v9 + d * v11) * invDet;
    res.data[3][2] = -(a * v8 - b * v10 + c * v11) * invDet;

    res.data[0][3] = -(b * v12 - c * v13 + d * v14) * invDet;
    res.data[1][3] = (a * v12 - c * v15 + d * v16) * invDet;
    res.data[2][3] = -(a * v13 - b * v15 + d * v17) * invDet;
    res.data[3][3] = (a * v14 - b * v16 + c * v17) * invDet;

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

} // namespace avalon
