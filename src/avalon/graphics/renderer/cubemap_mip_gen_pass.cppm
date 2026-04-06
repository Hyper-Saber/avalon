module;
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
export module avalon.graphics:cubemap_mip_gen_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

class CubemapMipGenPass final : public RenderPass<CubemapMipGenPass> {
public:
  explicit CubemapMipGenPass(ShaderHandle shader, rhi::Extent2D extent)
      : m_shader(shader), m_extent(extent),
        m_mipLevels(Log2(m_extent.width) + 1) {}

  void Setup(RenderGraphBuilder &builder) override {
    m_inputHandle = builder.Read(kSkyboxCube, rhi::EResourceUsage::TransferSrc);

    AVALON_ASSERT_MSG(m_extent.width == m_extent.height,
                      "[CubemapMipGenPass] Cubemap must be square!");

    m_outputHandle = builder.SetExtent(m_extent)
                         .SetLayers(6)
                         .SetMipLevels(m_mipLevels)
                         .Write(kSkyboxMipmap,
                                rhi::EResourceUsage::ReadWrite |
                                    rhi::EResourceUsage::TransferDst,
                                rhi::EResourceUsage::TransferDst);
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
                   m_mipLevels);
    cmd.CopyImage(inputTexture, outputTexture, region);
    cmd.Transition(outputTexture, rhi::EResourceUsage::ReadWrite, 6,
                   m_mipLevels);

    // rhi::ImageBarrier initialBarrier{
    //     .texture = outputTexture,
    //     .oldLayout = MapUsageToLayout(rhi::EResourceUsage::TransferDst),
    //     .newLayout = MapUsageToLayout(rhi::EResourceUsage::ReadWrite),
    //     .srcAccess = MapUsageToAccess(rhi::EResourceUsage::TransferDst),
    //     .dstAccess = MapUsageToAccess(rhi::EResourceUsage::ReadWrite),
    //     .srcStage = MapUsageToStage(rhi::EResourceUsage::TransferDst,
    //                                 rhi::EPassType::Compute),
    //     .dstStage = MapUsageToStage(rhi::EResourceUsage::ReadWrite,
    //                                 rhi::EPassType::Compute),
    //     .baseMipLevel = 0,
    //     .levelCount = m_mipLevels,
    //     .baseArrayLayer = 0,
    //     .layerCount = 6,
    // };
    // cmd.PipelineBarrier(initialBarrier);

    cmd.BindPipeline(m_pipelineHandle, rhi::EPipelineBindPoint::Compute);

    for (uint32_t i = 0; i < m_mipLevels - 1; i++) {
      uint32_t srcLevel = i;
      uint32_t dstLevel = i + 1;

      uint32_t mipWidth = Max(1u, m_extent.width >> dstLevel);
      uint32_t mipHeight = Max(1u, m_extent.height >> dstLevel);

      uint32_t srcIndex = bindlessManager.RegisterTextureArray(
          outputTexture, EResourceUsage::ReadWrite, srcLevel);
      uint32_t dstIndex = bindlessManager.RegisterRWTextureArray(
          outputTexture, EResourceUsage::ReadWrite, dstLevel);

      struct CustomPush {
        uint32_t srcIndex;
        uint32_t dstIndex;
        uint32_t samplerIndex;
        uint32_t targetWidth;
        uint32_t targetHeight;
        float invSize;
        float paddings[kMaxCustomSlots - 6];
      } customPc = {srcIndex, dstIndex, m_samplerIndex, mipWidth, mipHeight};
      customPc.invSize = 1.0f / static_cast<float>(mipWidth);

      StandardPushConstant constant;
      std::memcpy(constant.customSlots, &customPc, sizeof(CustomPush));

      cmd.PushConstants(EShaderStage::Compute, 0, sizeof(StandardPushConstant),
                        &constant);

      uint32_t groupX = (mipWidth + 7) / 8;
      uint32_t groupY = (mipHeight + 7) / 8;
      cmd.Dispatch(groupX, groupY, 6);

      rhi::ImageBarrier barrier{
          .texture = outputTexture,
          .srcAccess = rhi::EAccess::ShaderWrite,
          .dstAccess = rhi::EAccess::ShaderRead,
          .srcStage = rhi::EPipelineStage::ComputeShader,
          .dstStage = rhi::EPipelineStage::ComputeShader,
          .baseMipLevel = dstLevel,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 6,
      };

      cmd.PipelineBarrier(barrier);
    }
  }

private:
  ShaderHandle m_shader;
  rhi::Extent2D m_extent;

  uint32_t m_mipLevels = 1;
  uint32_t m_samplerIndex = 0;

  VirtualResourceHandle m_inputHandle;
  VirtualResourceHandle m_outputHandle;
  PipelineHandle m_pipelineHandle;
};

} // namespace avalon::graphics
