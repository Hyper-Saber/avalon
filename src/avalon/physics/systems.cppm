module;
export module avalon.physics:systems;
import :components;

import avalon.core;
import avalon.ecs;

export namespace avalon::ecs {
class PhysicsSystem final : public ecs::SystemBase<PhysicsSystem> {
  void OnUpdate(ecs::World &world, float dt) override {
    auto view = world.GetView<RigidBodyComponent, TransformComponent>();
    view.ForEach([&](auto &rigidBody, auto &transform) {
      if (LengthSquared(rigidBody.velocity) > kEpsilon)
        transform.SetPosition(transform.local.position +
                              rigidBody.velocity * dt);

      float angularSpeed = Length(rigidBody.angularVelocity);
      if (angularSpeed > kEpsilon) {
        float angle = angularSpeed * dt;
        Vec3 axis = Normalize(rigidBody.angularVelocity);

        auto delta = Quaternion::FromAxisAngle(axis, angle);
        auto nextRotation = delta * transform.local.rotation;
        nextRotation.Normalize();

        transform.SetRotation(nextRotation);
      }

      if (rigidBody.linearDamping > 0.f) {
        rigidBody.velocity =
            rigidBody.velocity * Max(0.f, 1.f - rigidBody.linearDamping * dt);
      }
      if (rigidBody.angularDamping > 0.f) {
        rigidBody.angularVelocity =
            rigidBody.angularVelocity *
            Max(0.f, 1.f - rigidBody.angularDamping * dt);
      }
    });
  }
};
} // namespace avalon::ecs
