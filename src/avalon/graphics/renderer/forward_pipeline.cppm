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
import :cubemap_mip_gen_pass;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API ForwardPipeline final
    : public RenderPipelineBase<ForwardPipeline> {
public:
  explicit ForwardPipeline(rhi::IRhi &rhi) : m_rhi(rhi) {}

  bool Initialize() override {
    auto &shaderManager = graphics::GetShaderManager();
    auto &materialManager = graphics::GetMaterialManager();

    auto shaderHandle = shaderManager.GetOrCreateShader("lit.hlsl");
    auto materialHandle =
        materialManager.CreateMaterial(shaderHandle, "Default"_id);
    materialManager.SetDefaultOpaque(materialHandle);

    shaderHandle = shaderManager.GetOrCreateShader("blit.hlsl");
    materialHandle = materialManager.CreateMaterial(shaderHandle, "Blit"_id);
    materialManager.SetDefaultBlit(materialHandle);

    shaderHandle = shaderManager.GetOrCreateShader("skybox.hlsl");
    materialHandle = materialManager.CreateMaterial(shaderHandle, "Skybox"_id);
    materialManager.SetDefaultSkyBox(materialHandle);

    shaderHandle = shaderManager.GetOrCreateShader("skybox_generator.hlsl");
    materialHandle =
        materialManager.CreateMaterial(shaderHandle, "SkyboxGen"_id);

    shaderHandle = shaderManager.GetOrCreateShader("cubemap_test.hlsl");
    materialHandle =
        materialManager.CreateMaterial(shaderHandle, "CubemapTest"_id);

    m_mipGenShader =
        shaderManager.GetOrCreateComputeShader("cubemap_mip_gen.hlsl");

    // TODO:: create physical pipeline

    return true;
  }

  StringId GetName() const override { return "ForwardPipeline"_id; }

  void Setup(RenderGraphBuilder &builder, const RenderPacket &packet) override {
    rhi::Extent2D skyboxExtent = {1024, 1024};
    builder.AddPass<SkyboxGeneratorPass>("SkyboxGen"_id, EPassType::Graphics,
                                         skyboxExtent);
    builder.AddPass<CubemapMipGenPass>("SkyboxMipmapGen"_id,
                                       rhi::EPassType::Compute, m_mipGenShader,
                                       skyboxExtent);
    builder.AddPass<OpaquePass>("Opaque"_id);
    builder.AddPass<SkyboxPass>("Skybox"_id);
    builder.AddPass<BlitPass>("Blit"_id);
  }

private:
  rhi::IRhi &m_rhi;

  ShaderHandle m_mipGenShader;
};

} // namespace avalon::graphics
