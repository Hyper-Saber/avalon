module;
export module avalon.scene:light_component;

import :types;
import avalon.core;

export namespace avalon::ecs {

struct LightComponent {
  scene::ELightType type = scene::ELightType::Directional;
  Color color = Color(1.f, 1.f, 1.f, 1.f);
  float intensity = 1.f;
  float range = 10.f;
  float spotInnerRadians = 1.f;
  float spotOuterRadians = 2.f;
};
} // namespace avalon::ecs
