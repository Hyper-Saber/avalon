module;
export module avalon.graphics:skybox_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

class SkyboxPass final : public RenderPass<SkyboxPass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    m_colorHandle =
        builder.SetLoadOp(rhi::EAttachmentLoadOp::Load)
            .WriteAttachment(kSceneColor, rhi::EResourceUsage::ColorAttachment);
    m_depthHandle =
        builder.SetLoadOp(rhi::EAttachmentLoadOp::Load)
            .WriteAttachment(kSceneDepth,
                             rhi::EResourceUsage::DepthStencilAttachment |
                                 rhi::EResourceUsage::ReadOnly);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto &materialManager = GetMaterialManager();
    auto material = materialManager.Resolve(materialManager.GetDefaultSkybox());
    material->SetDepthComplieOp(rhi::ECompareOp::GreaterOrEqual);
    material->DisableDepthWrite();
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    const auto &handle = GetMaterialManager().GetDefaultSkybox();
    if (!handle.IsValid()) [[unlikely]]
      return;

    auto material = GetMaterialManager().Resolve(handle);

    auto pipeline = material->GetOrCreatePipeline(
        context.rhi, context.pipelineRenderingInfo);

    cmd.BindPipeline(pipeline);
    cmd.Draw(3, 1, 0, 0);
  }

private:
  VirtualResourceHandle m_colorHandle;
  VirtualResourceHandle m_depthHandle;
};

} // namespace avalon::graphics
