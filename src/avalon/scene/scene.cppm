module;
#include <utility>
export module avalon.scene:scene;

import avalon.ecs;
import avalon.core;
import avalon.rhi;
import avalon.graphics;
import :components;
import :utils;

export namespace avalon::scene {
class AVALON_SCENE_API Scene final : public mem::AutoDestroyable<Scene> {
public:
  Scene() { m_world = MakeUnique<ecs::World>(); }

  auto GetWorld() const noexcept -> ecs::World & { return *m_world.Get(); }

  void Render(graphics::Renderer &renderer, rhi::ICommandBuffer &cmd) {
    auto snapshot = CaptureActiveCamera();
    graphics::SceneGlobals globals{
        .camera = snapshot,
    };

    auto view = m_world->GetView<ecs::LightComponent>();
    view.ForEach([&](ecs::Entity entity, ecs::LightComponent &light) {
      globals.lightData.color = light.color;
      globals.lightData.dirOrPos = light.directionOrPosition;
      globals.lightData.type =
          std::underlying_type_t<ELightType>(light.lightType);
    });

    renderer.Render(cmd, GetWorld(), globals);
  }

  auto CreatePrimitive(graphics::EPrimitiveType type) {
    return Utils::CreatePrimitive(*m_world.Get(), type);
  }

private:
  auto CaptureActiveCamera() -> graphics::CameraSnapshot {
    auto view =
        m_world->GetView<ecs::CameraComponent, ecs::TransformComponent>();
    for (auto entity : view) {
      auto transComp = view.Get<ecs::TransformComponent>(entity);
      return {
          .view = std::move(CalculateViewMatrix(transComp.local.position,
                                                transComp.local.rotation)),
          .projection = view.Get<ecs::CameraComponent>(entity).projectionMatrix,
          .position = Vec4::FromVec3(transComp.GetWorldPosition()),
      };
    }

    return {{}, Matrix4x4::Identity};
  }

  UniquePtr<ecs::World> m_world;
};
} // namespace avalon::scene
