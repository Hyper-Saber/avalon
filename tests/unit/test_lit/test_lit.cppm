module;
#include <expected>
#include <iostream>

export module test;

import test.utils;
import avalon.engine;
import avalon.core;
import avalon.rhi;
import avalon.scene;
import avalon.ecs;
import avalon.graphics;
import avalon.shader;
import avalon.window;
import :systems;
import :components;
import :input_system;

using namespace avalon;

class App : public ApplicationBase<App> {
public:
  void OnInitialize(scene::Scene &scene, rhi::IRhi &rhi,
                    rhi::Extent2D extent) override {
    auto &materialManager = graphics::GetMaterialManager();
    auto material = materialManager.Resolve(materialManager.GetDefaultOpaque());
    material->SetCullMode(rhi::ECullMode::Back);

    auto &world = scene.GetWorld();

    auto cube = scene.CreatePrimitive(graphics::ESDFType::Cube);
    auto sphere = scene.CreatePrimitive(graphics::ESDFType::Sphere);
    world.AddComponent<ecs::SphereComponent>(sphere);
    world.AddComponent<ecs::CubeComponent>(cube);

    auto materialInstanceHandle = materialManager.CreateMaterialInstance(
        materialManager.GetDefaultOpaque());
    auto materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("uMaterials.albedo"_id, Color::Red());
    materialInstance->SetProperty("uMaterials.metallic"_id, 0.9f);
    materialInstance->SetProperty("uMaterials.roughness"_id, 0.1f);
    materialInstance->SetProperty("uMaterials.ao"_id, 1.0f);

    auto renderComp = world.GetComponent<ecs::RenderComponent>(cube);
    renderComp->materialInstanceHandle = materialInstanceHandle;

    materialInstanceHandle = materialManager.CreateMaterialInstance(
        materialManager.GetDefaultOpaque());
    materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("uMaterials.albedo"_id, Color::Black());
    materialInstance->SetProperty("uMaterials.metallic"_id, 0.7f);
    materialInstance->SetProperty("uMaterials.roughness"_id, 0.3f);
    materialInstance->SetProperty("uMaterials.ao"_id, 1.0f);

    renderComp = world.GetComponent<ecs::RenderComponent>(sphere);
    renderComp->materialInstanceHandle = materialInstanceHandle;

    auto transComp = world.GetComponent<ecs::TransformComponent>(cube);
    transComp->SetPosition({0, 0, 1});
    transComp = world.GetComponent<ecs::TransformComponent>(sphere);
    transComp->SetPosition({0.5, 0, -1});

    auto light = scene.AddLight(scene::ELightType::Directional);
    transComp = world.GetComponent<ecs::TransformComponent>(light);
    transComp->SetRotation({-45, 0, 0});
    m_camera = scene.AddCamera();
    transComp = world.GetComponent<ecs::TransformComponent>(m_camera);
    transComp->SetPosition(Vec3{0, 0, 10});
    ecs::RigidBodyComponent rigidBody{
        .linearDamping = 20,
        .acceleration = 100,
    };
    world.AddComponent<ecs::RigidBodyComponent>(m_camera, rigidBody);

    world.AddSystem<ecs::UpdateLightSystem>();
    // world.AddSystem<ecs::MoveCubeSystem>();
    world.AddSystem<ecs::InputSystem>();

    auto &inputManager = input::GetInputManager();

    inputManager.LoadMapping(
        std::move(input::InputMapping::CreateDefaultDrone()));

    inputManager.BindAction("IncreaseMetallic"_id, input::EGamepadButton::A);
    inputManager.BindAction("DecreaseMetallic"_id, input::EGamepadButton::B);
    inputManager.BindAction("IncreaseRoughness"_id, input::EGamepadButton::X);
    inputManager.BindAction("DecreaseRoughness"_id, input::EGamepadButton::Y);
  }

private:
  UniquePtr<scene::Scene> m_scene;
  rhi::PipelineHandle m_pipeline;
  graphics::MeshHandle m_mesh;
  ecs::Entity m_model;
  ecs::Entity m_camera;
};

void TestHelloTriangle() {
  EngineConfig config{
      .renderDeviceRequirement =
          {
              .queueRequirement =
                  {
                      .isRequireGraphics = true,
                      .isRequireTransfer = true,
                      .isRequirePresent = true,
                  },
              .requiredCapabilities =
                  {
                      rhi::ERenderCapability::Swapchain,
                  },
          },
      .windowProps =
          {
              .width = 1920,
              .height = 1080,
          },
  };

  auto &engine = Engine::Get();
  auto app = MakeUnique<App>();
  auto initRes = engine.Initialize(config, std::move(app));

  test::Assert(initRes.has_value(), "Engine Initialization");

  engine.Run();

  engine.Clear();

  std::cout << "Test Finished Successfully!" << std::endl;
}

extern "C++" int main() {
  TestHelloTriangle();
  return 0;
}
