module;
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:prefilter_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class AVALON_GRAPHICS_API CubemapMipGenPass final
    : public RenderPass<CubemapMipGenPass> {
public:
  explicit CubemapMipGenPass(ShaderHandle shader, uint32_t faceDataOffset,
                             rhi::Extent2D extent, uint32_t mipLevels,
                             StringId sourceCubemap, StringId outputCubemap)
      : m_shader(shader), m_extent(extent), m_sourceCubemap(sourceCubemap),
        m_outputCubemap(outputCubemap), m_mipLevels(mipLevels) {}

  void Setup(RenderGraphBuilder &builder) override {
    m_inputHandle =
        builder.Read(m_sourceCubemap, EResourceUsage::TransferSrc,
                     EResourceUsage::TransferSrc, EShaderStage::Compute);

    AVALON_ASSERT_MSG(m_extent.width == m_extent.height,
                      "[CubemapMipGenPass] Cubemap must be square!");

    m_outputHandle =
        builder.SetExtent(m_extent)
            .SetLayers(6)
            .SetTextureType(rhi::ETextureType::TextureCube)
            .SetMipLevels(m_mipLevels)
            .Write(m_outputCubemap,
                   EResourceUsage::ReadWrite | EResourceUsage::TransferDst,
                   EResourceUsage::TransferDst, EShaderStage::Compute);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto pShader = GetShaderManager().Resolve(m_shader);
    AVALON_ASSERT_MSG(pShader->HasComputeStage(),
                      "[CubemapMipGenPass] Shader is not a Compute Shader!");
    ComputePipelineCreateInfo info{
        .stageInfo = *pShader->GetComputeStageInfo(),
        .descriptorSetLayoutBindings = pShader->GetDescriptorSetLayouts(),
    };
    m_pipelineHandle = rhi.GetOrCreateComputePipeline(info);
    m_samplerIndex = rhi.GetStaticSamplers().linearClamp;
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &bindlessManager = context.rhi.GetBindlessManager();
    auto inputTexture = context.GetPhysicalTexture(m_inputHandle);
    auto outputTexture = context.GetPhysicalTexture(m_outputHandle);

    ImageCopyRegion region{
        .extent = m_extent,
        .srcMipLevel = 0,
        .dstMipLevel = 0,
        .srcLayer = 0,
        .dstLayer = 0,
        .layerCount = 6,
    };

    cmd.Transition(outputTexture, rhi::EResourceUsage::TransferDst, 6,
                   m_mipLevels, EShaderStage::Compute);
    cmd.CopyImage(inputTexture, outputTexture, region);
    cmd.Transition(outputTexture, rhi::EResourceUsage::ReadWrite, 6,
                   m_mipLevels, EShaderStage::Compute);

    cmd.BindPipeline(m_pipelineHandle);

    for (uint32_t i = 0; i < m_mipLevels - 1; i++) {
      uint32_t srcLevel = i;
      uint32_t dstLevel = i + 1;

      uint32_t mipWidth = Max(1u, m_extent.width >> dstLevel);
      uint32_t mipHeight = Max(1u, m_extent.height >> dstLevel);
      float invSize = 1.0f / static_cast<float>(mipWidth);
      float roughness = static_cast<float>(i + 1) / (m_mipLevels - 1);

      uint32_t srcIndex = bindlessManager.RegisterTextureCube(outputTexture);
      uint32_t dstIndex = bindlessManager.RegisterRWTextureArray(
          outputTexture, EResourceUsage::ReadWrite, dstLevel);

      uint32_t finalSampleCount;
      if (i == 0)
        finalSampleCount = 1024;
      // else if (i < 3)
      //   finalSampleCount = 1024;
      else
        finalSampleCount = 1024;

      struct CustomPush {
        uint32_t srcIndex;
        uint32_t dstIndex;
        uint32_t samplerIndex;
        uint32_t sampleCount;
        uint32_t size;
        float invSize;
        float roughness;
        float paddings[kPushConstantFloatSize - 7];
      } customPc = {
          .srcIndex = srcIndex,
          .dstIndex = dstIndex,
          .samplerIndex = m_samplerIndex,
          .sampleCount = finalSampleCount,
          .size = mipWidth,
          .invSize = invSize,
          .roughness = roughness,
      };

      cmd.PushConstants(EShaderStage::Compute, 0, sizeof(CustomPush),
                        &customPc);

      uint32_t groupX = (mipWidth + 7) / 8;
      uint32_t groupY = (mipHeight + 7) / 8;
      cmd.Dispatch(groupX, groupY, 6);
    }
  }

private:
  ShaderHandle m_shader;
  rhi::Extent2D m_extent;

  StringId m_sourceCubemap;
  StringId m_outputCubemap;

  uint32_t m_mipLevels = 1;
  uint32_t m_samplerIndex = 0;

  VirtualResourceHandle m_inputHandle;
  VirtualResourceHandle m_outputHandle;
  PipelineHandle m_pipelineHandle;
};

} // namespace avalon::graphics
