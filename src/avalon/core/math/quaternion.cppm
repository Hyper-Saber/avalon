module;

export module avalon.core:math.quaternion;

import :math;
import :math.vector;
import :math.matrix;
import :string;

export namespace avalon {

class AVALON_CORE_API Quaternion {
public:
  float x = 0, y = 0, z = 0, w = 1;

  constexpr Quaternion() = default;
  constexpr Quaternion(float x, float y, float z, float w)
      : x(x), y(y), z(z), w(w) {}

  static Quaternion FromAxisAngle(const Vec3 &axis, float angleRadians);
  static Quaternion FromEuler(const Vec3 &euler);
  static Quaternion FromMatrix(const Matrix4x4 &m);
  static Quaternion LookRotation(const Vec3 &forward, const Vec3 &up);

  Vec3 ToEuler() const;
  Quaternion operator*(const Quaternion &other) const;
  void Normalize();
  Vec3 Rotate(const Vec3 &v) const;
  Matrix4x4 ToMatrix() const;
  String ToString() const;

private:
  static Quaternion FromGlm(const struct glm_quat_wrapper &gq);
};

} // namespace avalon
