module avalon.core;

import :math.quaternion;
import :math;

namespace avalon {
Quaternion Quaternion::LookRotation(const Vec3 &forward, const Vec3 &up) {
  Vec3 f = forward.Normalized();
  Vec3 r = Vec3::Cross(up, f).Normalized();
  Vec3 u = Vec3::Cross(f, r);

  Matrix4x4 m = Matrix4x4::Identity;

  m.data[0][0] = r.x;
  m.data[0][1] = r.y;
  m.data[0][2] = r.z;
  m.data[1][0] = u.x;
  m.data[1][1] = u.y;
  m.data[1][2] = u.z;
  m.data[2][0] = -f.x;
  m.data[2][1] = -f.y;
  m.data[2][2] = -f.z;

  return FromMatrix(m);
}
} // namespace avalon
