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
    material->SetCullMode(rhi::ECullMode::None);
    auto &world = scene.GetWorld();

    auto cube = scene.CreatePrimitive(graphics::EPrimitiveType::Cube);
    auto sphere = scene.CreatePrimitive(graphics::EPrimitiveType::Sphere);
    auto plane = scene.CreatePrimitive(graphics::EPrimitiveType::Plane);
    world.AddComponent<ecs::CubeComponent>(cube);

    auto materialInstanceHandle = materialManager.CreateMaterialInstance(
        materialManager.GetDefaultOpaque());
    auto materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("uMaterials.baseColor"_id, Color::Red());
    materialInstance->SetProperty("uMaterials.specularColor"_id,
                                  Color::Yellow());
    materialInstance->SetProperty("uMaterials.shininess"_id, 20.0f);
    materialInstance->SetProperty("uMaterials.f0"_id, 0.04f);
    auto renderComp = world.GetComponent<ecs::RenderComponent>(cube);
    renderComp->materialInstanceHandle = materialInstanceHandle;
    auto meshHandle = renderComp->meshHandle;
    graphics::GetMeshManager().UploadMesh(meshHandle,
                                          material->GetVertexLayout());

    materialInstanceHandle = materialManager.CreateMaterialInstance(
        materialManager.GetDefaultOpaque());
    materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("uMaterials.baseColor"_id, Color::Blue());
    materialInstance->SetProperty("uMaterials.specularColor"_id, Color::Cyan());
    materialInstance->SetProperty("uMaterials.shininess"_id, 100.0f);
    materialInstance->SetProperty("uMaterials.f0"_id, 0.4f);

    renderComp = world.GetComponent<ecs::RenderComponent>(sphere);
    renderComp->materialInstanceHandle = materialInstanceHandle;
    meshHandle = renderComp->meshHandle;
    graphics::GetMeshManager().UploadMesh(meshHandle,
                                          material->GetVertexLayout());

    materialInstanceHandle = materialManager.CreateMaterialInstance(
        materialManager.GetDefaultOpaque());
    materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("uMaterials.baseColor"_id, Color::White());
    materialInstance->SetProperty("uMaterials.specularColor"_id, Color::Blue());
    materialInstance->SetProperty("uMaterials.shininess"_id, 50.0f);
    materialInstance->SetProperty("uMaterials.f0"_id, 0.1f);

    renderComp = world.GetComponent<ecs::RenderComponent>(plane);
    renderComp->materialInstanceHandle = materialInstanceHandle;
    meshHandle = renderComp->meshHandle;
    graphics::GetMeshManager().UploadMesh(meshHandle,
                                          material->GetVertexLayout());

    auto transComp = world.GetComponent<ecs::TransformComponent>(cube);
    transComp->SetPosition({0, 0, 1});
    transComp = world.GetComponent<ecs::TransformComponent>(sphere);
    transComp->SetPosition({0.5, 0, -1});

    transComp = world.GetComponent<ecs::TransformComponent>(plane);
    transComp->SetPosition({0, -1, 0});
    transComp->SetScale({10, 1, 10});

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
    world.AddSystem<ecs::MoveCubeSystem>();
    world.AddSystem<ecs::InputSystem>();

    input::GetInputManager().LoadMapping(
        std::move(input::InputMapping::CreateDefaultDrone()));
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
