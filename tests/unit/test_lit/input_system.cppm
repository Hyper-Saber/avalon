module;
export module test:input_system;

import avalon.core;
import avalon.ecs;
import avalon.engine;
import avalon.physics;
import avalon.graphics;
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

    auto isIncreasingRoughness =
        inputManager.IsActionHolding("IncreaseRoughness"_id);
    auto isDecreasingRoughness =
        inputManager.IsActionHolding("DecreaseRoughness"_id);
    auto isIncreasingMetallic =
        inputManager.IsActionHolding("IncreaseMetallic"_id);
    auto isDecreasingMetallic =
        inputManager.IsActionHolding("DecreaseMetallic"_id);

    auto yValue = ascendValue - descendValue;
    auto rollValue = 0.0f;
    rollValue += isRollingLeft ? -1.0f : 0.0f;
    rollValue += isRollingRight ? 1.0f : 0.0f;
    auto dRoughness = 0.0f;
    auto dMetallic = 0.0f;
    dRoughness += isIncreasingRoughness ? 1.0f : 0.0f;
    dRoughness += isDecreasingRoughness ? -1.0f : 0.0f;
    dMetallic += isIncreasingMetallic ? 1.0f : 0.0f;
    dMetallic += isDecreasingMetallic ? -1.0f : 0.0f;

    static float roughness = 0.5f;
    static float metallic = 0.5f;
    dRoughness *= dt;
    dMetallic *= dt;
    roughness += dRoughness;
    metallic += dMetallic;
    roughness = Clamp01(roughness);
    metallic = Clamp01(metallic);
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

    auto shpereView =
        world.GetView<ecs::SphereComponent, ecs::RenderComponent>();
    shpereView.Foreach([&](auto &sphere, auto &render) {
      auto &mm = graphics::GetMaterialManager();
      auto materialInstance = mm.Resolve(render.materialInstanceHandle);
      materialInstance->SetProperty("uMaterials.roughness"_id, roughness);
      materialInstance->SetProperty("uMaterials.metallic"_id, metallic);
    });
  }
};

} // namespace avalon::ecs
