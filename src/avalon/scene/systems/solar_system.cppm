export module avalon.scene:solar_system;

import avalon.core;
import avalon.ecs;
import :light_component;

namespace avalon::ecs {

export struct SolarComponent {
  float latitude = 30.f;
  float declination = 0.0f;
  float timeOfDay = 6.0f;
  float timeMultiplier = 10.0f;

  float intensity = 10.0f;
  Color color = Color(1.0f, 1.0f, 1.0f);
  Color horizonColor = Color(1.0f, 0.4f, 0.15f, 1.0f);
};

class SolarSystem final : public ecs::SystemBase<SolarSystem> {
public:
  void OnUpdate(ecs::World &world, float dt) override {
    auto &context = GetContext();

    auto view =
        world.GetView<TransformComponent, LightComponent, SolarComponent>();
    view.Foreach([&](auto &transform, auto &light, auto &solar) {
      float minutesPerTick = dt * solar.timeMultiplier;
      solar.timeOfDay += minutesPerTick / 60.0f;
      if (solar.timeOfDay >= 24.0f)
        solar.timeOfDay -= 24.0f;

      float phi = ToRadians(solar.latitude);
      float delta = ToRadians(solar.declination);
      float H = (solar.timeOfDay / 24.0f - 0.5f) * 2.0f * kPi;

      float sinAltitude =
          Sin(phi) * Sin(delta) + Cos(phi) * Cos(delta) * Cos(H);
      sinAltitude = Clamp(sinAltitude, -1.0f, 1.0f);

      float xEast = -Cos(delta) * Sin(H);
      float zSouth = Sin(phi) * Cos(delta) * Cos(H) - Cos(phi) * Sin(delta);

      Vec3 sunPos = Normalize(Vec3(xEast, sinAltitude, zSouth));

      transform.LookAt(-sunPos, Vec3::Up());

      UpdateAtmosphere(light, solar, sunPos.y);
    });
  }

private:
  void UpdateAtmosphere(LightComponent &light, const SolarComponent &solar,
                        float sunHeight) {
    float horizonFade = SmoothStep(-0.1f, 0.1f, sunHeight);
    if (sunHeight > 0.0f) {
      light.intensity =
          solar.intensity * horizonFade * SmoothStep(0.0f, 0.2f, sunHeight);

      light.color = Color::Lerp(solar.horizonColor, solar.color,
                                SmoothStep(0.0f, 0.4f, sunHeight));
    } else {
      light.intensity = 0.05f;
      light.color = Color(0.12f, 0.15f, 0.25f);
    }
  }
};
} // namespace avalon::ecs
