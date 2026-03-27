module;
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

class BlitPass final : public RenderPass<BlitPass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    m_outputHandle =
        builder.Write(kSwapchainColor, rhi::EResourceUsage::Present);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    m_blitMaterialHandle = GetMaterialManager().GetDeafaultBlit();
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto *material = GetMaterialManager().Resolve(m_blitMaterialHandle);
    if (!material)
      return;

    auto pipeline = material->GetOrCreatePipeline(
        context.rhi, context.pipelineRenderingInfo);
    cmd.BindPipeline(pipeline);

    cmd.Draw(3, 1, 0, 0);
  }

private:
  VirtualResourceHandle m_inputHandle;
  VirtualResourceHandle m_outputHandle;
  MaterialHandle m_blitMaterialHandle;
};

} // namespace avalon::graphics
