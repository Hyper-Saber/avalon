export module avalon.core:transform;

import :math;

export namespace avalon {

struct Transform {
  Vec3 position = {0.0f, 0.0f, 0.0f};
  Vec3 rotation = {0.0f, 0.0f, 0.0f};
  Vec3 scale = {1.0f, 1.0f, 1.0f};

  Matrix4x4 GetMatrix() const {
    Matrix4x4 res;

    res = Translate(res, position);

    res = RotateY(res, rotation.y);
    res = RotateX(res, rotation.x);
    res = RotateZ(res, rotation.z);

    res = Scale(res, scale);

    return res;
  }

  Vec3 Forward() const {
    Matrix4x4 m = GetMatrix();
    return Normalize(Vec3{-m.data[2][0], -m.data[2][1], -m.data[2][2]});
  }

  Vec3 Right() const {
    Matrix4x4 m = GetMatrix();
    return Normalize(Vec3{m.data[0][0], m.data[0][1], m.data[0][2]});
  }

  Vec3 Up() const {
    Matrix4x4 m = GetMatrix();
    return Normalize(Vec3{m.data[1][0], m.data[1][1], m.data[1][2]});
  }
};

} // namespace avalon
