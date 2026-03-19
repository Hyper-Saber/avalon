module;
#include <expected>
#include <iostream>

export module test;

import test.utils;
import avalon.engine;
import avalon.core;
import avalon.shader;
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
        Path(vfs::kShaderFolderVirtualPath) / StringView("test_mvp.hlsl"));

    auto &materialManager = graphics::GetMaterialManager();
    auto materialHandle =
        materialManager.CreateMaterial(shaderHandle, "default"_id);
    auto material = materialManager.Resolve(materialHandle);
    materialManager.SetDefaultMaterial(materialHandle);

    auto pipelineCreateInfo = material->GetPipelineCreateInfo();
    pipelineCreateInfo.renderPassHandle = renderPass;

    m_pipeline = rhi.CreatePipeline(pipelineCreateInfo);

    auto renderer = MakeUnique<graphics::Renderer>(rhi, m_pipeline);

    renderer->AddPass(MakeUnique<graphics::OpaquePass>(m_pipeline, renderPass,
                                                       shaderHandle, extent));

    Engine::Get().SetRenderer(std::move(renderer));

    m_model = scene.CreatePrimitive(graphics::EPrimitiveType::Cube);

    auto meshComponent =
        scene.GetWorld().GetComponent<ecs::RenderComponent>(m_model);
    auto handle = meshComponent->meshHandle;
    graphics::GetMeshManager().UploadMesh(handle, material->GetVertexLayout());

    auto &world = scene.GetWorld();
    m_camera = world.CreateEntity();
    world.AddComponent<ecs::CameraComponent>(m_camera);
    auto transform = Transform{
        .position = {0, 0, 5},
        .scale = Vec3::One(),
    };
    world.AddComponent<ecs::TransformComponent>(m_camera, transform);
    world.AddSystem<ecs::CameraSystem>();
    world.AddSystem<ecs::UpdateTransformSystem>();
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
