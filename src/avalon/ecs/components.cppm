export module avalon.ecs:components;
import avalon.core;
export namespace avalon::ecs {
struct TransformComponent {
  Transform local;
  Matrix4x4 worldMatrix;
  bool isDirty = true;

  void SetPosition(const Vec3 &p) {
    local.position = p;
    isDirty = true;
  }

  void SetRotation(const Vec3 &r) {
    local.rotation = r;
    isDirty = true;
  }

  void SetScale(const Vec3 &s) {
    local.scale = s;
    isDirty = true;
  }

  Vec3 GetWorldPosition() {
    UpdateWorldMatrix();
    return Vec3{worldMatrix.data[3][0], worldMatrix.data[3][1],
                worldMatrix.data[3][2]};
  }

  Vec3 GetWorldPosition() const {
    return Vec3{worldMatrix.data[3][0], worldMatrix.data[3][1],
                worldMatrix.data[3][2]};
  }

  void UpdateWorldMatrix() {
    if (!isDirty)
      return;
    isDirty = false;
    auto T = Translate(local.position);
    auto R = Rotate(local.rotation);
    auto S = Scale(local.scale);
    worldMatrix = T * R * S;
  }

  Vec3 Forward() const {
    return Normalize(Vec3{-worldMatrix.data[2][0], -worldMatrix.data[2][1],
                          -worldMatrix.data[2][2]});
  }

  Vec3 Right() const {
    return Normalize(Vec3{worldMatrix.data[0][0], worldMatrix.data[0][1],
                          worldMatrix.data[0][2]});
  }

  Vec3 Up() const {
    return Normalize(Vec3{worldMatrix.data[1][0], worldMatrix.data[1][1],
                          worldMatrix.data[1][2]});
  }
};

} // namespace avalon::ecs
