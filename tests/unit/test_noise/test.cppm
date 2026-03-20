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

    rhi::RenderPassCreateInfo passCreateInfo{
        .colorAttachments = {colorAttachment},
    };

    auto renderPass = rhi.CreateRenderPass(passCreateInfo);
    Engine::Get().SetMainPass(renderPass);

    auto shaderHandle = graphics::GetShaderManager().GetOrCreateShader(
        Path(vfs::kShaderFolderVirtualPath) / StringView("test_noise.hlsl"));

    auto &materialManager = graphics::GetMaterialManager();
    auto materialHandle =
        materialManager.CreateMaterial(shaderHandle, "default"_id);
    auto material = materialManager.Resolve(materialHandle);
    materialManager.SetDefaultMaterial(materialHandle);

    auto pipelineCreateInfo = material->GetPipelineCreateInfo();
    pipelineCreateInfo.renderPassHandle = renderPass;

    m_pipeline = rhi.CreatePipeline(pipelineCreateInfo);

    auto renderer = MakeUnique<graphics::Renderer>(rhi, m_pipeline);

    renderer->AddPass(MakeUnique<graphics::FullscreenPass>(m_pipeline, renderPass,
                                                       shaderHandle, extent));

    Engine::Get().SetRenderer(std::move(renderer));
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
