module;
export module avalon.scene:visibility_system;

import avalon.ecs;
import avalon.graphics;
import avalon.core;
import :camera_component;
import :types;

namespace avalon::ecs {
class VisibilitySystem final : public RenderSystemBase<VisibilitySystem> {
  void OnCapture(World &world, graphics::SceneSnapshot &outSnapshot) override {
    graphics::Frustum frustum;
    bool cameraFound = false;

    auto camView =
        world.GetView<ecs::CameraComponent, ecs::TransformComponent>();
    for (auto entity : camView) {
      auto &cam = camView.Get<ecs::CameraComponent>(entity);
      auto &trans = camView.Get<ecs::TransformComponent>(entity);

      outSnapshot.camera.view = CalculateViewMatrix(trans.GetWorldPosition(),
                                                    trans.GetRotationEuler());
      outSnapshot.camera.projection = cam.projectionMatrix;
      outSnapshot.camera.viewProjection =
          outSnapshot.camera.projection * outSnapshot.camera.view;
      outSnapshot.camera.invView = outSnapshot.camera.view.Inverse();
      outSnapshot.camera.invProjection = cam.inverseProjectionMatrix;
      outSnapshot.camera.invViewProjection =
          outSnapshot.camera.viewProjection.Inverse();
      outSnapshot.camera.worldPosition =
          Vec4::FromVec3(trans.GetWorldPosition());

      frustum.Update(outSnapshot.camera.viewProjection);
      cameraFound = true;
      break;
    }

    if (!cameraFound)
      return;

    auto renderView =
        world.GetView<ecs::RenderComponent, ecs::TransformComponent>();
    for (auto entity : renderView) {
      auto &render = renderView.Get<ecs::RenderComponent>(entity);
      auto &trans = renderView.Get<ecs::TransformComponent>(entity);
      AABB worldBounds = render.localBounds.Transform(trans.worldMatrix);

      if (frustum.IsVisible(worldBounds)) {
        outSnapshot.opaqueMeshHandles.PushBack({render.meshHandle.id});
        outSnapshot.opaqueMaterials.PushBack({render.materialHandle.id});
        outSnapshot.opaqueMaterialInstances.PushBack(
            {render.materialInstanceHandle.id});
        outSnapshot.opaqueWorldMatrices.PushBack(trans.worldMatrix);
      }
    }
  }
};
} // namespace avalon::ecs
