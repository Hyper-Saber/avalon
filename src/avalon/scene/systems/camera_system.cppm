module;
export module avalon.scene:update_camera_projection_system;

import avalon.core;
import avalon.ecs;
import :types;
import :components;

namespace avalon::ecs {
class UpdateCameraProjectionSystem final : public SystemBase<UpdateCameraProjectionSystem> {
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
