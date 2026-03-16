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

    m_model = CreateGeometryEntity(scene, material);

    auto &world = scene.GetWorld();
    m_camera = world.CreateEntity();
    world.AddComponent<ecs::CameraComponent>(m_camera);
    auto transform = Transform{
        .position = {0, 0, 5},
        .rotation = {0, 180, 0},
        .scale = Vec3::One(),
    };
    world.AddComponent<ecs::TransformComponent>(m_camera, transform);
    world.AddSystem<ecs::CameraSystem>();
    world.AddSystem<ecs::UpdateTransformSystem>();
  }

private:
  ecs::Entity CreateGeometryEntity(scene::Scene &scene,
                                   const graphics::Material &material) {
    graphics::MeshData data{.positions{// Front face (Z = 0.5)
                                       {-0.5f, -0.5f, 0.5f},
                                       {0.5f, -0.5f, 0.5f},
                                       {0.5f, 0.5f, 0.5f},
                                       {-0.5f, 0.5f, 0.5f},
                                       // Back face (Z = -0.5)
                                       {-0.5f, -0.5f, -0.5f},
                                       {0.5f, -0.5f, -0.5f},
                                       {0.5f, 0.5f, -0.5f},
                                       {-0.5f, 0.5f, -0.5f}},
                            .indices{// Front
                                     0, 1, 2, 2, 3, 0,
                                     // Right
                                     1, 5, 6, 6, 2, 1,
                                     // Back
                                     7, 6, 5, 5, 4, 7,
                                     // Left
                                     4, 0, 3, 3, 7, 4,
                                     // Top
                                     3, 2, 6, 6, 7, 3,
                                     // Bottom
                                     4, 5, 1, 1, 0, 4},
                            .colors{
                                {1, 1, 1},
                                {1, 1, 1},
                                {1, 1, 1},
                                {1, 1, 1}, // 前四个顶点颜色
                                {1, 1, 1},
                                {1, 1, 1},
                                {1, 1, 1},
                                {1, 1, 1} // 后四个顶点颜色
                            },
                            .texCoords{{0.f, 0.f},
                                       {1.f, 0.f},
                                       {1.f, 1.f},
                                       {0.f, 1.f},
                                       {0.f, 0.f},
                                       {1.f, 0.f},
                                       {1.f, 1.f},
                                       {0.f, 1.f}}};

    m_mesh =
        graphics::GetMeshManager().CreateMesh(data, material.GetVertexLayout());

    auto entity = scene.GetWorld().CreateEntity();
    Transform transform;
    scene.GetWorld()
        .AddComponent<ecs::MeshComponent>(entity, m_mesh)
        ->AddComponent<ecs::TransformComponent>(entity, transform);

    return entity;
  }

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
