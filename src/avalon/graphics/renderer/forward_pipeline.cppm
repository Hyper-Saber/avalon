module;
#include <cstdint>
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
import :skybox_gen_pass;
import :cubemap_mip_gen_pass;
import :brdf_lut_gen_pass;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API ForwardPipeline final
    : public RenderPipelineBase<ForwardPipeline> {
public:
  explicit ForwardPipeline(rhi::IRhi &rhi) : m_rhi(rhi) {}

  bool Initialize() override {

    rhi::ProbeData data = graphics::CreateSkyboxProbeData();
    m_rhi.UpdateProbeBuffer(0, &data, sizeof(rhi::ProbeData));

    struct FaceData {
      Vec4 right;
      Vec4 up;
      Vec4 forward;
    } faces[6];

    uint32_t i = 0;
    for (auto &view : data.captureViews) {
      faces[i].right = Vec4::FromVec3(view.GetRight());
      faces[i].up = Vec4::FromVec3(view.GetUp());
      faces[i++].forward = Vec4::FromVec3(view.GetBack());
    }

    auto allocation = m_rhi.GetSSBOPool().AllocateAligned(sizeof(FaceData) * 6);
    m_faceDataOffset = allocation.offset;

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

    m_brdfLutGenShader =
        shaderManager.GetOrCreateComputeShader("brdf_lut_gen.hlsl");
    m_mipGenShader =
        shaderManager.GetOrCreateComputeShader("cubemap_mip_gen.hlsl");

    TextureCreateInfo info{
        .nameHash = "BRDFLut"_id,
        .width = 512,
        .height = 512,
        .format = rhi::EFormat::R16G16_SFLOAT,
        .usage = rhi::EResourceUsage::ReadWrite | rhi::EResourceUsage::ReadOnly,
    };

    m_brdfLutDesc = {
        .nameHash = "BRDFLut"_id,
        .usage = rhi::EResourceUsage::ReadWrite | rhi::EResourceUsage::ReadOnly,
        .format = rhi::EFormat::R16G16_SFLOAT,
        .extent = {512, 512},
    };
    m_brdfLut = m_rhi.CreateTexture(info);

    // TODO:: create physical pipeline

    return true;
  }

  StringId GetName() const override { return "ForwardPipeline"_id; }

  void Setup(RenderGraphBuilder &builder, const RenderPacket &packet) override {

    builder.ImportExternalTexture(m_brdfLutDesc.nameHash, m_brdfLut,
                                  m_brdfLutDesc);

    if (m_isFisrtFrame) [[unlikely]] {
      builder.AddPass<BRDFLutGenPass>("BRDFLutGen"_id, EPassType::Compute,
                                      m_brdfLutGenShader, m_brdfLutDesc);
      m_isFisrtFrame = false;
    }

    rhi::Extent2D skyboxExtent = {1024, 1024};
    builder.AddPass<SkyboxGeneratorPass>("SkyboxGen"_id, EPassType::Graphics,
                                         skyboxExtent);
    builder.AddPass<CubemapMipGenPass>(
        "SkyboxMipmapGen"_id, rhi::EPassType::Compute, m_mipGenShader,
        m_faceDataOffset, skyboxExtent, "Skybox"_id, "SkyboxMipmap"_id);
    builder.AddPass<OpaquePass>("Opaque"_id);
    builder.AddPass<SkyboxPass>("Skybox"_id);
    builder.AddPass<BlitPass>("Blit"_id);
  }

private:
  bool m_isFisrtFrame = true;

  VirtualTextureDesc m_brdfLutDesc;
  TextureHandle m_brdfLut;

  rhi::IRhi &m_rhi;
  uint32_t m_faceDataOffset;
  ShaderHandle m_mipGenShader;
  ShaderHandle m_brdfLutGenShader;
};

} // namespace avalon::graphics
