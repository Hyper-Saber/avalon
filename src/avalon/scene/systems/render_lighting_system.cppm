module;
#include <utility>
export module avalon.scene:render_lighting_system;

import avalon.ecs;
import avalon.graphics;
import avalon.core;
import :light_component;
import :types;

namespace avalon::ecs {
class RenderLightingSystem final
    : public RenderSystemBase<RenderLightingSystem> {

  void OnCapture(World &world, graphics::SceneSnapshot &outSnapshot) override {
    auto view = world.GetView<TransformComponent, LightComponent>();

    for (auto entity : view) {
      const auto &transform = view.Get<TransformComponent>(entity);
      const auto &light = view.Get<LightComponent>(entity);

      auto &data = outSnapshot.lightData;

      data.SetColor(light.color.ToVec3(), light.intensity);

      data.type = std::to_underlying(light.type);
      data.range = light.range;

      if (light.type == scene::ELightType::Directional) {
        Vec3 dir = transform.worldMatrix.GetForward();
        data.SetDirection(dir);
      } else {
        Vec3 pos = transform.worldMatrix.GetTranslation();
        data.SetPosition(pos);

        if (light.type == scene::ELightType::Spot) {
          data.spotInnerCosine = Cos(light.spotInnerRadians);
          data.spotOuterCosine = Cos(light.spotOuterRadians);
        }
      }
      break;
    }
  }
};
} // namespace avalon::ecs
