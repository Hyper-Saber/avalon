module;
#include <cstdint>
export module avalon.core:math.matrix;

import :string;

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

} // namespace avalon
