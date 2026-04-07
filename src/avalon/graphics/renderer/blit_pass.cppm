module;
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:blit_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class AVALON_GRAPHICS_API BlitPass final : public RenderPass<BlitPass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    m_outputHandle =
        builder.Write(kSwapchainColor, rhi::EResourceUsage::Present);
    m_inputHandle = builder.Read(kSceneColor, rhi::EResourceUsage::ReadOnly);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto &mm = GetMaterialManager();
    auto handle = mm.GetDefaultBlit();
    m_material = mm.Resolve(handle);
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    if (!m_material)
      return;

    auto pipeline = m_material->GetOrCreatePipeline(
        context.rhi, context.pipelineRenderingInfo);
    cmd.BindPipeline(pipeline);

    auto handle = context.GetPhysicalTexture(m_inputHandle);
    auto textureIndex =
        context.rhi.GetBindlessManager().RegisterTexture(handle);

    struct CustomPush {
      uint32_t sceneColor;
      uint32_t sampler;
      float paddings[kPushConstantFloatSize - 2];
    } customPush{
        .sceneColor = textureIndex,
        .sampler = context.rhi.GetStaticSamplers().linearClamp,
    };

    cmd.PushConstants(rhi::EShaderStage::All, 0, sizeof(StandardPushConstant),
                      &customPush);

    cmd.Draw(3, 1, 0, 0);
  }

private:
  VirtualResourceHandle m_inputHandle;
  VirtualResourceHandle m_outputHandle;
  Material *m_material = nullptr;
  MaterialInstance *m_materialInstance = nullptr;
};

} // namespace avalon::graphics
