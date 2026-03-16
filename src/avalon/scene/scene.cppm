module;
#include <utility>
export module avalon.scene:scene;

import avalon.ecs;
import avalon.core;
import avalon.rhi;
import avalon.graphics;
import :components;

export namespace avalon::scene {
class AVALON_SCENE_API Scene final : public mem::AutoDestroyable<Scene> {
public:
  Scene() { m_world = MakeUnique<ecs::World>(); }

  auto GetWorld() const noexcept -> ecs::World & { return *m_world.Get(); }

  void Render(graphics::Renderer &renderer, rhi::ICommandBuffer &cmd) {
    auto snapshot = CaptureActiveCamera();

    renderer.Render(cmd, GetWorld(), snapshot);
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
          .position = transComp.GetWorldPosition(),
      };
    }

    return {{}, Matrix4x4::Identity};
  }

  UniquePtr<ecs::World> m_world;
};
} // namespace avalon::scene
