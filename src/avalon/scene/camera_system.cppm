module;
export module avalon.scene:camera_system;

import avalon.core;
import avalon.ecs;
import :types;
import :components;

export namespace avalon::ecs {
class AVALON_SCENE_API CameraSystem final : public SystemBase<CameraSystem> {
  void OnUpdate(World &world, float dt) override {
    auto view = world.GetView<CameraComponent>();
    view.Foreach([&](Entity entity, CameraComponent &camera) {
      if (camera.isDirty) {
        if (camera.projectionType == scene::EProjectionType::Perspective) {
          camera.UpdateProjectionMatrix();
        }
      }
    });
  }
};

} // namespace avalon::ecs
