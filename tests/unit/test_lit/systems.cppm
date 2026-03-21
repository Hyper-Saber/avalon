module;
export module test:systems;

import avalon.core;
import avalon.ecs;
import avalon.engine;
import avalon.graphics;
import avalon.scene;
import :components;

namespace avalon::ecs {

class MoveCubeSystem : public ecs::SystemBase<MoveCubeSystem> {
  void OnUpdate(ecs::World &world, float dt) override {
    auto view = world.GetView<TransformComponent, CubeComponent>();
    auto totalTime = Engine::Get().GetTotalTime();
    view.Foreach([&](auto &transComp, auto &_) {
      auto rotation = transComp.GetRotationEuler();
      rotation.x = totalTime * 10;
      rotation.y = totalTime * 10;
      rotation.z = totalTime * 10;
      transComp.SetRotation(rotation);
    });
  }
};

class UpdateLightSystem : public ecs::SystemBase<UpdateLightSystem> {
  void OnUpdate(ecs::World &world, float dt) override {
    auto view = world.GetView<ecs::LightComponent>();

    float rotationSpeed = 45.0f;
    float angleDelta = ToRadians(rotationSpeed * dt);

    float cosA = Cos(angleDelta);
    float sinA = Sin(angleDelta);

    view.Foreach([&](auto &light) {
      if (light.lightType == scene::ELightType::Directional) {
        float x = light.directionOrPosition.x;
        float y = light.directionOrPosition.y;

        float newX = x * cosA - y * sinA;
        float newY = x * sinA + y * cosA;

        light.directionOrPosition.x = newX;
        light.directionOrPosition.y = newY;

        Vec3 dir =
            Normalize({light.directionOrPosition.x, light.directionOrPosition.y,
                       light.directionOrPosition.z});

        light.directionOrPosition.x = dir.x;
        light.directionOrPosition.y = dir.y;
        light.directionOrPosition.z = dir.z;
      }
    });
  }
};
} // namespace avalon::ecs
