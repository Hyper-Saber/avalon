module;
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:sdf_shadow_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class SDFShadowPass final : public RenderPass<SDFShadowPass> {
public:
  explicit SDFShadowPass(ShaderHandle shaderHandle)
      : m_shaderHandle(shaderHandle) {}

  void Setup(RenderGraphBuilder &builder) override {
    m_outputHandle =
        builder.SetFormat(EFormat::R8G8B8A8_UNORM)
            .Write("ShadowMask"_id, EResourceUsage::ReadWrite,
                   EResourceUsage::ReadWrite, EShaderStage::Compute);
    m_depthHandle =
        builder.Read(kSceneDepth, EResourceUsage::ReadOnly,
                     EResourceUsage::ReadOnly, EShaderStage::Compute);
    m_instanceIDHandle =
        builder.Read("InstanceId"_id, EResourceUsage::ReadOnly,
                     EResourceUsage::ReadOnly, EShaderStage::Compute);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto pShader = GetShaderManager().Resolve(m_shaderHandle);
    AVALON_ASSERT_MSG(pShader->HasComputeStage(),
                      "[CubemapMipGenPass] Shader is not a Compute Shader!");
    ComputePipelineCreateInfo info{
        .stageInfo = *pShader->GetComputeStageInfo(),
        .descriptorSetLayoutBindings = pShader->GetDescriptorSetLayouts(),
    };
    m_pipelineHandle = rhi.GetOrCreateComputePipeline(info);
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &packet = context.renderPacket;
    if (packet.opaqueBatches.IsEmpty())
      return;

    auto &bindlessManager = context.rhi.GetBindlessManager();

    auto depthTextureHandle = context.GetPhysicalTexture(m_depthHandle);
    auto shadowMaskHandle = context.GetPhysicalTexture(m_outputHandle);
    auto instanceIDHandle = context.GetPhysicalTexture(m_instanceIDHandle);

    auto shadowMaskIndex = bindlessManager.RegisterRWTexture(
        shadowMaskHandle, EResourceUsage::ReadWrite);
    auto depthTextureIndex =
        bindlessManager.RegisterTexture(depthTextureHandle);
    auto instanceIDTextureIndex =
        bindlessManager.RegisterTexture(instanceIDHandle);

    cmd.BindPipeline(m_pipelineHandle);

    struct CustomPush {
      uint32_t depthTextureIndex;
      uint32_t instanceIDTextureIndex;
      uint32_t shadowMaskIndex;
      uint32_t width;
      uint32_t height;
      float invWidth;
      float invHeight;
      float paddings[kPushConstantFloatSize - 7];
    } customPush{
        .depthTextureIndex = depthTextureIndex,
        .instanceIDTextureIndex = instanceIDTextureIndex,
        .shadowMaskIndex = shadowMaskIndex,
        .width = context.resolution.width,
        .height = context.resolution.height,
        .invWidth = context.resolution.invWidth,
        .invHeight = context.resolution.invHeight,
    };

    cmd.PushConstants(EShaderStage::Compute, 0, sizeof(CustomPush),
                      &customPush);

    cmd.Dispatch((context.resolution.width + 7) / 8,
                 (context.resolution.height + 7) / 8, 1);
  }

private:
  VirtualResourceHandle m_depthHandle;
  VirtualResourceHandle m_instanceIDHandle;
  VirtualResourceHandle m_outputHandle;
  ShaderHandle m_shaderHandle;
  PipelineHandle m_pipelineHandle;
};
} // namespace avalon::graphics
