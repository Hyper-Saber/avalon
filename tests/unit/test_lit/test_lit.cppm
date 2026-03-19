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

using namespace avalon;

class App : public ApplicationBase<App> {
public:
  void OnInitialize(scene::Scene &scene, rhi::IRhi &rhi,
                    rhi::Extent2D extent) override {
    rhi::AttachmentDescription colorAttachment{
        .format = rhi.GetSwapchainImageFormat(),
        .loadOp = rhi::EAttachmentLoadOp::Clear,
        .storeOp = rhi::EAttachmentStoreOp::Store,
        .initialLayout = rhi::EResourceLayout::Undefined,
        .finalLayout = rhi::EResourceLayout::Present,
    };

    rhi::AttachmentDescription depthAttachment{
        .format = rhi::EFormat::D32_SFLOAT_S8_UINT,
        .loadOp = rhi::EAttachmentLoadOp::Clear,
        .storeOp = rhi::EAttachmentStoreOp::DontCare,
        .initialLayout = rhi::EResourceLayout::Undefined,
        .finalLayout = rhi::EResourceLayout::DepthStencilAttachment,
    };

    rhi::RenderPassCreateInfo passCreateInfo{
        .colorAttachments = {colorAttachment},
        .depthAttachment = depthAttachment,
        .hasDepth = true,
    };

    auto renderPass = rhi.CreateRenderPass(passCreateInfo);
    Engine::Get().SetMainPass(renderPass);

    auto shaderHandle = graphics::GetShaderManager().GetOrCreateShader(
        Path(vfs::kShaderFolderVirtualPath) / StringView("lit.hlsl"));
    auto &materialManager = graphics::GetMaterialManager();
    auto materialHandle =
        materialManager.CreateMaterial(shaderHandle, "default"_id);
    materialManager.SetDefaultMaterial(materialHandle);
    auto material = materialManager.Resolve(materialHandle);

    auto pipelineCreateInfo = material->GetPipelineCreateInfo();
    pipelineCreateInfo.renderPassHandle = renderPass;

    m_pipeline = rhi.CreatePipeline(pipelineCreateInfo);

    auto renderer = MakeUnique<graphics::Renderer>(rhi, m_pipeline);

    renderer->AddPass(MakeUnique<graphics::OpaquePass>(m_pipeline, renderPass,
                                                       shaderHandle, extent));

    Engine::Get().SetRenderer(std::move(renderer));

    auto &world = scene.GetWorld();
    m_camera = world.CreateEntity();
    world.AddComponent<ecs::CameraComponent>(m_camera);
    auto transform = Transform{
        .position = {0, 0, 5},
        .scale = Vec3::One(),
    };
    world.AddComponent<ecs::TransformComponent>(m_camera, transform);
    auto light = world.CreateEntity();
    world.AddComponent<ecs::LightComponent>(light);

    auto lightComp = world.GetComponent<ecs::LightComponent>(light);
    lightComp->directionOrPosition = Vec4::FromVec3(Vec3{0, -1, -1});

    auto cube = scene.CreatePrimitive(graphics::EPrimitiveType::Cube);
    auto sphere = scene.CreatePrimitive(graphics::EPrimitiveType::Sphere);

    world.AddComponent<ecs::CubeComponent>(cube);

    auto renderComp = world.GetComponent<ecs::RenderComponent>(cube);
    auto meshHandle = renderComp->meshHandle;
    graphics::GetMeshManager().UploadMesh(meshHandle,
                                          material->GetVertexLayout());
    auto materialInstanceHandle = renderComp->materialInstanceHandle;
    auto materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("mMaterial.baseColor"_id, Color::Red());
    materialInstance->SetProperty("mMaterial.specularColor"_id, Color::Cyan());
    materialInstance->SetProperty("mMaterial.shininess"_id, 0.f);
    materialInstance->SetProperty("mMaterial.f0"_id, 0.04f);

    renderComp = world.GetComponent<ecs::RenderComponent>(sphere);
    meshHandle = renderComp->meshHandle;
    graphics::GetMeshManager().UploadMesh(meshHandle,
                                          material->GetVertexLayout());
    materialInstanceHandle = renderComp->materialInstanceHandle;
    materialInstance = materialManager.Resolve(materialInstanceHandle);
    materialInstance->SetProperty("mMaterial.baseColor"_id, Color::Blue());
    materialInstance->SetProperty("mMaterial.specularColor"_id, Color::Green());
    materialInstance->SetProperty("mMaterial.shininess"_id, 200.f);
    materialInstance->SetProperty("mMaterial.f0"_id, 0.5f);

    auto transComp = world.GetComponent<ecs::TransformComponent>(cube);
    transComp->SetPosition(Vec3{-1, 0, 0});
    transComp = world.GetComponent<ecs::TransformComponent>(sphere);
    transComp->SetPosition(Vec3{1, 0, 0});

    world.AddSystem<ecs::CameraSystem>();
    world.AddSystem<ecs::UpdateLightSystem>();
    world.AddSystem<ecs::MoveCubeSystem>();
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
              .width = 800,
              .height = 600,
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
