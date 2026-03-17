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
import :systems;

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
    auto material = graphics::Material(shaderHandle);

    auto pipelineCreateInfo = material.GetPipelineCreateInfo();
    pipelineCreateInfo.renderPassHandle = renderPass;

    m_pipeline = rhi.CreatePipeline(pipelineCreateInfo);

    auto renderer = MakeUnique<graphics::Renderer>(rhi, m_pipeline);

    renderer->AddPass(
        MakeUnique<graphics::OpaquePass>(m_pipeline, renderPass, extent));

    Engine::Get().SetRenderer(std::move(renderer));

    auto &world = scene.GetWorld();
    m_camera = world.CreateEntity();
    world.AddComponent<ecs::CameraComponent>(m_camera);
    auto transform = Transform{
        .position = {0, 0, 5},
        .scale = Vec3::One(),
    };
    world.AddComponent<ecs::TransformComponent>(m_camera, transform);
    world.AddSystem<ecs::CameraSystem>();
    world.AddSystem<ecs::UpdateLightSystem>();

    auto light = world.CreateEntity();
    world.AddComponent<ecs::LightComponent>(light);

    auto lightComp = world.GetComponent<ecs::LightComponent>(light);
    lightComp->directionOrPosition = Vec4::FromVec3(Vec3{0, -1, -2});

    auto cube = scene.CreatePrimitive(graphics::EPrimitiveType::Cube);
    auto sphere = scene.CreatePrimitive(graphics::EPrimitiveType::Sphere);
    auto plane = scene.CreatePrimitive(graphics::EPrimitiveType::Plane);

    auto handle = world.GetComponent<ecs::MeshComponent>(cube)->meshHandle;
    graphics::GetMeshManager().UploadMesh(handle, material.GetVertexLayout());
    handle = world.GetComponent<ecs::MeshComponent>(sphere)->meshHandle;
    graphics::GetMeshManager().UploadMesh(handle, material.GetVertexLayout());
    handle = world.GetComponent<ecs::MeshComponent>(plane)->meshHandle;
    graphics::GetMeshManager().UploadMesh(handle, material.GetVertexLayout());

    auto transComp = world.GetComponent<ecs::TransformComponent>(cube);
    transComp->SetPosition(Vec3{-1, 0, 0});
    transComp = world.GetComponent<ecs::TransformComponent>(sphere);
    transComp->SetPosition(Vec3{1, 0, 0});
    transComp = world.GetComponent<ecs::TransformComponent>(plane);
    transComp->SetPosition(Vec3{0, -1, 0});
    transComp->SetScale(Vec3(5, 1, 5));
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
