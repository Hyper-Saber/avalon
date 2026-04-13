module;
#include <cstddef>
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:sh_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class AVALON_GRAPHICS_API SHPass final : public RenderPass<SHPass> {
  static const uint32_t internalSize = 16;

public:
  explicit SHPass(ShaderHandle shShader, ShaderHandle finalizeSHShader,
                  rhi::Extent2D extent, StringId sourceCubemap,
                  StringId dstBuffer)
      : m_shShader(shShader), m_finalizeSHShader(finalizeSHShader),
        m_sourceCubemap(sourceCubemap), m_sampleLevel(Log2(extent.width / 16)),
        m_dstBuffer(dstBuffer) {}

  void Setup(RenderGraphBuilder &builder) override {
    m_inputHandle =
        builder.Read(m_sourceCubemap, rhi::EResourceUsage::ReadOnly,
                     rhi::EResourceUsage::ReadOnly, EShaderStage::Compute);

    m_ouputHandle =
        builder.WriteBuffer(m_dstBuffer, rhi::EResourceUsage::StorageBuffer,
                            rhi::EResourceUsage::TransferDst, sizeof(CubemapSH),
                            EShaderStage::None);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto pShader = GetShaderManager().Resolve(m_shShader);
    AVALON_ASSERT_MSG(pShader->HasComputeStage(),
                      "[CubemapMipGenPass] Shader is not a Compute Shader!");
    ComputePipelineCreateInfo info{
        .stageInfo = *pShader->GetComputeStageInfo(),
        .descriptorSetLayoutBindings = pShader->GetDescriptorSetLayouts(),
    };
    m_shPipelineHandle = rhi.GetOrCreateComputePipeline(info);

    pShader = GetShaderManager().Resolve(m_finalizeSHShader);
    AVALON_ASSERT_MSG(pShader->HasComputeStage(),
                      "[CubemapMipGenPass] Shader is not a Compute Shader!");
    ComputePipelineCreateInfo finalizeSHInfo{
        .stageInfo = *pShader->GetComputeStageInfo(),
        .descriptorSetLayoutBindings = pShader->GetDescriptorSetLayouts(),
    };
    m_finalizeSHPipelineHandle = rhi.GetOrCreateComputePipeline(finalizeSHInfo);
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &bindlessManager = context.rhi.GetBindlessManager();
    auto inputTexture = context.GetPhysicalTexture(m_inputHandle);

    auto inputIndex = bindlessManager.RegisterTextureCube(inputTexture);
    auto allocation = context.GetPhysicalBufferAllocation(m_ouputHandle);

    float invSize = 1.0f / internalSize;

    struct CustomPush {
      uint32_t envMap;
      uint32_t sampler;
      uint32_t outputOffset;
      uint32_t weightOffset;
      uint32_t sampleLevel;
      float invSize;
      float paddings[kPushConstantFloatSize - 6];
    } customPush{
        .envMap = inputIndex,
        .sampler = m_samplerIndex,
        .outputOffset = allocation.offset,
        .weightOffset = static_cast<uint32_t>(allocation.offset +
                                              offsetof(CubemapSH, weight)),
        .sampleLevel = m_sampleLevel,
        .invSize = invSize,
    };

    // Debug("begin sh pass, buffer: {}, offset: {}, size: {}, fill: {}",
    //       allocation.buffer.id, allocation.offset, allocation.size, 1);
    cmd.FillBuffer(allocation.buffer, allocation.offset, allocation.size, 0);

    cmd.SyncBuffer(allocation.buffer, EResourceUsage::ReadWrite,
                   allocation.offset, allocation.size, EShaderStage::Compute);

    cmd.BindPipeline(m_shPipelineHandle);
    cmd.PushConstants(EShaderStage::Compute, 0, sizeof(CustomPush),
                      &customPush);

    auto groupCount = internalSize / 8;
    cmd.Dispatch(groupCount, groupCount, 6);

    cmd.SyncBuffer(allocation.buffer, EResourceUsage::ReadWrite,
                   allocation.offset, allocation.size, EShaderStage::Compute);

    struct FinalizePush {
      uint32_t offset;
      uint32_t weightOffset;
      float paddings[kPushConstantFloatSize - 2];
    } finalizePush{
        .offset = allocation.offset,
        .weightOffset = static_cast<uint32_t>(allocation.offset +
                                              offsetof(CubemapSH, weight)),
    };

    cmd.BindPipeline(m_finalizeSHPipelineHandle);
    cmd.PushConstants(EShaderStage::Compute, 0, sizeof(FinalizePush),
                      &finalizePush);

    cmd.Dispatch(1, 1, 1);
  }

private:
  ShaderHandle m_shShader;
  ShaderHandle m_finalizeSHShader;

  StringId m_sourceCubemap;
  StringId m_dstBuffer;

  uint32_t m_sampleLevel = 1;
  uint32_t m_samplerIndex = 0;

  VirtualResourceHandle m_inputHandle;
  VirtualResourceHandle m_ouputHandle;
  PipelineHandle m_shPipelineHandle;
  PipelineHandle m_finalizeSHPipelineHandle;
};

} // namespace avalon::graphics
