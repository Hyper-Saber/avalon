module;
export module avalon.physics:components;

import avalon.core;

export namespace avalon::ecs {
struct RigidBodyComponent {
  Vec3 velocity = Vec3::Zero();
  Vec3 angularVelocity = Vec3::Zero();

  float mass = 1.0f;
  float linearDamping = 0.0f;
  float angularDamping = 0.0f;
};

} // namespace avalon::ecs
