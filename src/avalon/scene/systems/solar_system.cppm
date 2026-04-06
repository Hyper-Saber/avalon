export module avalon.scene:solar_system;

import avalon.core;
import avalon.ecs;
import :light_component;

namespace avalon::ecs {

export struct SolarComponent {
  float latitude = 30.f;
  float declination = 0.0f;
  float timeOfDay = 8.0f;
  float timeMultiplier = 1.0f;

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
    float visibility = SmoothStep(-0.2f, 0.15f, sunHeight);

    float exposureComp = Lerp(2.0f, 1.0f, SmoothStep(0.0f, 0.3f, sunHeight));

    if (sunHeight > -0.2f) {
      light.intensity = solar.intensity * visibility * exposureComp;
      light.color = solar.color;
    } else {
      light.intensity = 0.02f;
      light.color = Color(0.1f, 0.15f, 0.3f);
    }
  }
};
} // namespace avalon::ecs
