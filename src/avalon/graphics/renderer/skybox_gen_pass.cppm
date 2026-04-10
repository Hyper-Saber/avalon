module;
export module avalon.graphics:skybox_gen_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

class SkyboxGeneratorPass final : public RenderPass<SkyboxGeneratorPass> {
public:
  explicit SkyboxGeneratorPass(rhi::Extent2D extent) : m_extent(extent) {}

  void Setup(RenderGraphBuilder &builder) override {
    builder.SetViewMask(0x3F).SetLayers(6);
    m_cubeHandle =
        builder.SetLayers(6)
            .SetTextureType(rhi::ETextureType::TextureCube)
            .SetExtent(m_extent)
            .WriteAttachment("Skybox"_id, rhi::EResourceUsage::ColorAttachment);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto &materialManager = GetMaterialManager();
    m_material =
        materialManager.Resolve(materialManager.TryGetMaterial("SkyboxGen"_id));
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto pipeline = m_material->GetOrCreatePipeline(
        context.rhi, context.pipelineRenderingInfo);
    cmd.BindPipeline(pipeline);
    cmd.Draw(3, 1, 0, 0);
  }

private:
  rhi::Extent2D m_extent;
  VirtualResourceHandle m_cubeHandle;
  Material *m_material = nullptr;
};

} // namespace avalon::graphics
