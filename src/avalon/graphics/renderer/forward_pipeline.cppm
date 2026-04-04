export module avalon.graphics:forward_pipeline;

import avalon.core;
import avalon.rhi;
import :render_pipeline;
import :render_graph_builder;
import :renderer_types;
import :material;
import :material_manager;
import :opaque_pass;
import :skybox_pass;
import :blit_pass;
import :cubemap_test_pass;
import :skybox_gen_pass;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API ForwardPipeline final
    : public RenderPipelineBase<ForwardPipeline> {
public:
  explicit ForwardPipeline(rhi::IRhi &rhi) : m_rhi(rhi) {}

  StringId GetName() const override { return "ForwardPipeline"_id; }

  void Setup(RenderGraphBuilder &builder, const RenderPacket &packet) override {
    builder.AddPass<SkyboxGeneratorPass>("SkyboxGen"_id);
    builder.AddPass<OpaquePass>("Opaque"_id);
    builder.AddPass<SkyboxPass>("Skybox"_id);
    builder.AddPass<BlitPass>("Blit"_id);
  }

private:
  rhi::IRhi &m_rhi;
};

} // namespace avalon::graphics
