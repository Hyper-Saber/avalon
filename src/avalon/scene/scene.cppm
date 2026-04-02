module;
#include <utility>
export module avalon.scene:scene;

import avalon.core;
import avalon.ecs;
import avalon.core;
import avalon.rhi;
import avalon.graphics;
import :light_component;
import :camera_component;
import :utils;
import :visibility_system;
import :extract_light_system;
import :update_camera_projection_system;

export namespace avalon::scene {

class AVALON_SCENE_API Scene final : public mem::AutoDestroyable<Scene> {
public:
  Scene() {
    m_world = MakeUnique<ecs::World>();
    m_world->AddSystem<ecs::UpdateCameraProjectionSystem>();
    m_world->AddSystem<ecs::VisibilitySystem>();
    m_world->AddSystem<ecs::ExtractLightSystem>();
  }

  void Update(float dt) { m_world->Update(dt); }

  auto GetWorld() const noexcept -> ecs::World & { return *m_world.Get(); }

  void Render(graphics::IRenderer &renderer) {
    graphics::SceneSnapshot snapshot;
    m_world->Capture(snapshot);
    renderer.Render(snapshot);
  }

  auto CreatePrimitive(graphics::EPrimitiveType type) {
    return Utils::CreatePrimitive(*m_world.Get(), type);
  }

  ecs::Entity AddLight(ELightType type) {
    auto entity = m_world->CreateEntity();
    ecs::LightComponent lightComp{};
    ecs::TransformComponent transComp{};
    m_world->AddComponent<ecs::LightComponent>(entity, std::move(lightComp))
        .AddComponent<ecs::TransformComponent>(entity, transComp);

    return entity;
  }

  ecs::Entity AddCamera() {
    auto entity = m_world->CreateEntity();
    m_world->AddComponent<ecs::CameraComponent>(entity)
        .AddComponent<ecs::TransformComponent>(entity);
    return entity;
  }

private:
  UniquePtr<ecs::World> m_world;
};

} // namespace avalon::scene
