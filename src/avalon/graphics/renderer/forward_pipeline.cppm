module;
#include <cstddef>
#include <cstdint>
#include <cstring>
export module avalon.graphics:forward_pipeline;

import avalon.core;
import avalon.rhi;
import :render_pipeline;
import :render_graph_builder;
import :renderer_types;
import :material;
import :material_manager;
import :g_buffer_pass;
import :opaque_pass;
import :skybox_pass;
import :blit_pass;
import :skybox_gen_pass;
import :prefilter_pass;
import :sh_pass;
import :brdf_lut_gen_pass;
import :buffer_copy_pass;
import :sdf_shadow_pass;
import :types;

import :cubemap_test;

namespace {
constexpr uint32_t kSkyboxSize = 256;
} // namespace

export namespace avalon::graphics {

class AVALON_GRAPHICS_API ForwardPipeline final
    : public RenderPipelineBase<ForwardPipeline> {
public:
  explicit ForwardPipeline(rhi::IRhi &rhi) : m_rhi(rhi) {}

  bool Initialize() override {

    rhi::ProbeData data = graphics::CreateSkyboxProbeData();
    auto allocation = m_rhi.AllocateStaticSSBO(sizeof(rhi::ProbeData));
    std::memcpy(allocation.pHostAddress, &data, sizeof(rhi::ProbeData));

    uint32_t i = 0;
    for (auto &view : data.captureViews) {
      m_faces[i].right = Vec4::FromVec3(view.GetRight());
      m_faces[i].up = Vec4::FromVec3(view.GetUp());
      m_faces[i++].forward = Vec4::FromVec3(view.GetForward());
    }

    auto &shaderManager = graphics::GetShaderManager();
    auto &materialManager = graphics::GetMaterialManager();

    auto shaderHandle = shaderManager.GetOrCreateShader("g_buffer.hlsl");
    auto materialHandle =
        materialManager.CreateMaterial(shaderHandle, "GBuffer"_id);

    shaderHandle = shaderManager.GetOrCreateShader("skybox_generator.hlsl");
    materialHandle =
        materialManager.CreateMaterial(shaderHandle, "SkyboxGen"_id);

    shaderHandle = shaderManager.GetOrCreateShader("skybox.hlsl");
    materialHandle = materialManager.CreateMaterial(shaderHandle, "Skybox"_id);
    materialManager.SetDefaultSkyBox(materialHandle);

    shaderHandle = shaderManager.GetOrCreateShader("lit.hlsl");
    materialHandle = materialManager.CreateMaterial(shaderHandle, "Default"_id);
    auto pMat = materialManager.Resolve(materialHandle);
    pMat->DisableDepthWrite();
    pMat->SetDepthComplieOp(ECompareOp::GreaterOrEqual);
    materialManager.SetDefaultOpaque(materialHandle);

    shaderHandle = shaderManager.GetOrCreateShader("blit.hlsl");
    materialHandle = materialManager.CreateMaterial(shaderHandle, "Blit"_id);
    materialManager.SetDefaultBlit(materialHandle);

    m_shadowShader = shaderManager.GetOrCreateComputeShader("sdf_shadow.hlsl");
    m_brdfLutGenShader =
        shaderManager.GetOrCreateComputeShader("brdf_lut_gen.hlsl");
    m_mipGenShader =
        shaderManager.GetOrCreateComputeShader("cubemap_prefilter.hlsl");
    m_shShader =
        shaderManager.GetOrCreateComputeShader("cubemap_sh_projection.hlsl");
    m_finalizeSHShader =
        shaderManager.GetOrCreateComputeShader("finalize_sh.hlsl");

    //
    //
    //-------------------------------------------------------
    if constexpr (debug::kIsDebug) {
      shaderHandle = shaderManager.GetOrCreateShader("cubemap_test.hlsl");
      materialHandle =
          materialManager.CreateMaterial(shaderHandle, "CubemapTest"_id);
    }
    //-------------------------------------------------------
    //
    //

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

    TextureCreateInfo testInfo{
        .nameHash = "TestTexture"_id,
        .width = 8,
        .height = 8,
        .format = rhi::EFormat::R16G16B16A16_SFLOAT,
        .usage = rhi::EResourceUsage::ReadWrite | rhi::EResourceUsage::ReadOnly,
    };

    m_testTextureDesc = {
        .nameHash = "TestTexture"_id,
        .usage = rhi::EResourceUsage::ReadWrite | rhi::EResourceUsage::ReadOnly,
        .format = rhi::EFormat::R16G16B16A16_SFLOAT,
        .extent = {8, 8},
    };
    m_testTexture = m_rhi.CreateTexture(testInfo);

    // TODO:: create physical pipeline

    return true;
  }

  StringId GetName() const override { return "ForwardPipeline"_id; }

  void Setup(RenderGraphBuilder &builder, const RenderPacket &packet) override {

    builder.ImportExternalTexture(m_brdfLutDesc.nameHash, m_brdfLut,
                                  m_brdfLutDesc);
    builder.ImportExternalTexture(m_testTextureDesc.nameHash, m_testTexture,
                                  m_testTextureDesc);

    if (m_isFisrtFrame) [[unlikely]] {
      builder.AddPass<BRDFLutGenPass>("BRDFLutGen"_id, EPassType::Compute,
                                      m_brdfLutGenShader, m_brdfLutDesc);
      m_isFisrtFrame = false;
    }

    builder.AddPass<GBufferPass>("GBuffer"_id);
    builder.AddPass<SDFShadowPass>("Shadow"_id, EPassType::Compute,
                                   m_shadowShader);

    builder.AddPass<SkyboxGeneratorPass>("SkyboxGen"_id, EPassType::Graphics,
                                         skyboxExtent);

    auto allocation =
        m_rhi.GetDynamicSSBOPool().AllocateAligned(sizeof(FaceData) * 6);
    std::memcpy(allocation.pHostAddress, &m_faces, sizeof(FaceData) * 6);
    builder.AddPass<CubemapMipGenPass>(
        "SkyboxPrefilter"_id, rhi::EPassType::Compute, m_mipGenShader,
        allocation.offset, skyboxExtent, m_mipLevels, "Skybox"_id,
        "SkyboxPrefiltered"_id);
    builder.AddPass<SHPass>("SkyboxSH"_id, rhi::EPassType::Compute, m_shShader,
                            m_finalizeSHShader, skyboxExtent,
                            "SkyboxPrefiltered"_id, "SkyboxSH"_id);
    // builder.AddPass<BufferCopyPass>("BufferCopy"_id, rhi::EPassType::Compute,
    //                                 "SkyboxSH"_id, "SceneGlobals"_id,
    //                                 sizeof(CubemapSH), 0,
    //                                 offsetof(SceneGlobals, skyboxSH));
    builder.AddPass<OpaquePass>("Opaque"_id);
    // builder.AddPass<CubemapTestPass>("CubemapTest"_id);
    builder.AddPass<SkyboxPass>("Skybox"_id);
    builder.AddPass<BlitPass>("Blit"_id);
  }

private:
  struct FaceData {
    Vec4 right;
    Vec4 up;
    Vec4 forward;
  };

  bool m_isFisrtFrame = true;

  VirtualTextureDesc m_brdfLutDesc;
  TextureHandle m_brdfLut;
  VirtualTextureDesc m_testTextureDesc;
  TextureHandle m_testTexture;

  rhi::Extent2D skyboxExtent = {kSkyboxSize, kSkyboxSize};
  uint32_t m_mipLevels = Log2(kSkyboxSize / 2) + 1;
  rhi::IRhi &m_rhi;

  ShaderHandle m_shadowShader;
  ShaderHandle m_mipGenShader;
  ShaderHandle m_brdfLutGenShader;
  ShaderHandle m_shShader;
  ShaderHandle m_finalizeSHShader;

  FaceData m_faces[6];
};

} // namespace avalon::graphics
