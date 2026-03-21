module;
#include <cmath>
export module test:systems;

import avalon.core;
import avalon.ecs;
import avalon.engine;
import avalon.graphics;

namespace avalon::ecs {
class UpdateTransformSystem : public ecs::SystemBase<UpdateTransformSystem> {
  void OnUpdate(ecs::World &world, float dt) override {
    auto view = world.GetView<ecs::TransformComponent, ecs::RenderComponent>();
    auto totalTime = Engine::Get().GetTotalTime();
    view.Foreach([&](auto &transComp, auto &_) {
      auto position = transComp.local.position;
      auto rotation = transComp.GetRotationEuler();
      position.x = std::sin(totalTime);
      position.y = std::cos(totalTime);
      rotation.x = totalTime * 60;
      rotation.y = totalTime * 60;
      rotation.z = totalTime * 60;
      transComp.SetPosition(position);
      transComp.SetRotation(rotation);
    });
  }
};
} // namespace avalon::ecs
