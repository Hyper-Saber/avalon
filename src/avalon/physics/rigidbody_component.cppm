module;
export module avalon.physics:rigidbody_component;

import avalon.core;

export namespace avalon::ecs {
struct RigidBodyComponent {
  Vec3 velocity = Vec3::Zero();
  Vec3 angularVelocity = Vec3::Zero();

  float mass = 1.0f;
  float linearDamping = 0.0f;
  float angularDamping = 0.0f;

  float acceleration = 50.0f;
  float angularAcceleration = 0.0f;
  float maxSpeed = 20.f;
  float maxSpeedSquared = 400.f;

  void ApplyImpulse(Vec3 impulse) {
    velocity += impulse / mass;
    ClampVelocity(velocity);
  }

  void ApplyForce(Vec3 force, float dt) {
    velocity += (force / mass) * dt;
    ClampVelocity(velocity);
  }

  void ClampVelocity(Vec3 velocity) {
    if (velocity.LengthSquared() > maxSpeedSquared) {
      this->velocity = velocity.Normalized() * maxSpeed;
    }
  }
};
} // namespace avalon::ecs
