module;
export module test:input_system;

import avalon.core;
import avalon.ecs;
import avalon.engine;
import avalon.physics;
import avalon.input;
import avalon.scene;
import :components;

namespace avalon::ecs {

class InputSystem : public ecs::SystemBase<InputSystem> {
  void OnUpdate(ecs::World &world, float dt) override {
    auto view = world.GetView<ecs::TransformComponent, ecs::CameraComponent,
                              ecs::RigidBodyComponent>();

    auto &inputManager = input::GetInputManager();
    auto ascendValue = inputManager.GetTriggerValue("Ascend"_id);
    auto descendValue = inputManager.GetTriggerValue("Descend"_id);
    auto isRollingLeft = inputManager.IsActionHolding("RollLeft"_id);
    auto isRollingRight = inputManager.IsActionHolding("RollRight"_id);
    auto moveInput = inputManager.GetAxisValue("FlightMove"_id);
    auto rotateInput = inputManager.GetAxisValue("CameraLook"_id);

    auto yValue = ascendValue - descendValue;
    auto rollValue = 0.0f;
    rollValue += isRollingLeft ? -1.0f : 0.0f;
    rollValue += isRollingRight ? 1.0f : 0.0f;

    view.Foreach([&](auto &transform, auto &camera, auto &rigidBody) {
      auto moveDir = -transform.Forward() * moveInput.y +
                     transform.Right() * moveInput.x + transform.Up() * yValue;
      rigidBody.ApplyForce(moveDir * rigidBody.acceleration, dt);
      camera.currentRotation.x -= rotateInput.y * camera.sensitivity * dt;
      camera.currentRotation.y -= rotateInput.x * camera.sensitivity * dt;
      camera.currentRotation.z -= rollValue * camera.sensitivity * dt;

      auto qYaw =
          Quaternion::FromAxisAngle(Vec3::Up(), camera.currentRotation.y);
      auto qPitch =
          Quaternion::FromAxisAngle(Vec3::Right(), camera.currentRotation.x);
      auto qRoll =
          Quaternion::FromAxisAngle(Vec3::Forward(), camera.currentRotation.z);
      transform.SetRotation(qYaw * qPitch * qRoll);
    });
  }
};

} // namespace avalon::ecs
