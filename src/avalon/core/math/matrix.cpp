module;
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

module avalon.core;
import :math.matrix;

namespace avalon {

const Matrix4x4 Matrix4x4::Identity =
    Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

Vec3 Matrix4x4::GetColumn(uint32_t col) const {
  return Vec3{data[col][0], data[col][1], data[col][2]};
}

Vec3 Matrix4x4::GetRight() const { return GetColumn(0); }
Vec3 Matrix4x4::GetLeft() const { return -GetColumn(0); }
Vec3 Matrix4x4::GetUp() const { return GetColumn(1); }
Vec3 Matrix4x4::GetDown() const { return -GetColumn(1); }
Vec3 Matrix4x4::GetForward() const { return -GetColumn(2); }
Vec3 Matrix4x4::GetBack() const { return GetColumn(2); }

void Matrix4x4::GetBasis(Vec3 &outRight, Vec3 &outUp, Vec3 &outForward) const {
  outRight = GetColumn(0);
  outUp = GetColumn(1);
  outForward = -GetColumn(2);
}

Vec3 Matrix4x4::GetTranslation() const {
  return Vec3{data[3][0], data[3][1], data[3][2]};
}

Vec4 Matrix4x4::operator*(const Vec4 &v) const {
  const auto &gMat = *reinterpret_cast<const glm::mat4 *>(this->data);
  const auto &gVec = *reinterpret_cast<const glm::vec4 *>(&v);
  glm::vec4 gRes = gMat * gVec;
  return *reinterpret_cast<Vec4 *>(&gRes);
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4 &b) const {
  const auto &gMatA = *reinterpret_cast<const glm::mat4 *>(this->data);
  const auto &gMatB = *reinterpret_cast<const glm::mat4 *>(b.data);
  glm::mat4 gRes = gMatA * gMatB;

  Matrix4x4 res;
  *reinterpret_cast<glm::mat4 *>(res.data) = gRes;
  return res;
}

Matrix4x4 Matrix4x4::Transpose() const {
  const auto &gMat = *reinterpret_cast<const glm::mat4 *>(this->data);
  glm::mat4 gRes = glm::transpose(gMat);

  Matrix4x4 res;
  *reinterpret_cast<glm::mat4 *>(res.data) = gRes;
  return res;
}

Matrix4x4 Matrix4x4::Inverse() const {
  const auto &gMat = *reinterpret_cast<const glm::mat4 *>(this->data);
  glm::mat4 gInv = glm::inverse(gMat);

  Matrix4x4 res;
  *reinterpret_cast<glm::mat4 *>(res.data) = gInv;
  return res;
}

String Matrix4x4::ToString() const {
  return String::Format("\n[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]\n"
                        "[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]\n"
                        "[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]\n"
                        "[{:>8.4f}, {:>8.4f}, {:>8.4f}, {:>8.4f}]",
                        data[0][0], data[1][0], data[2][0], data[3][0],
                        data[0][1], data[1][1], data[2][1], data[3][1],
                        data[0][2], data[1][2], data[2][2], data[3][2],
                        data[0][3], data[1][3], data[2][3], data[3][3]);
}

} // namespace avalon
