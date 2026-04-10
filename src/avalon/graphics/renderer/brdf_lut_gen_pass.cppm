module;
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:brdf_lut_gen_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class AVALON_GRAPHICS_API BRDFLutGenPass final
    : public RenderPass<BRDFLutGenPass> {
public:
  explicit BRDFLutGenPass(ShaderHandle shader, const VirtualTextureDesc &desc)
      : m_shader(shader), m_desc(desc) {}

  void Setup(RenderGraphBuilder &builder) override {
    m_outputHandle =
        builder.SetExtent(m_desc.extent)
            .SetFormat(m_desc.format)
            .Write(m_desc.nameHash, m_desc.usage, EResourceUsage::ReadWrite,
                   EShaderStage::Compute);
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
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &bindlessManager = context.rhi.GetBindlessManager();
    auto texture = context.GetPhysicalTexture(m_outputHandle);
    auto index = bindlessManager.RegisterRWTexture(
        texture, rhi::EResourceUsage::ReadWrite);

    float invSize = 1.0f / m_desc.extent.width;
    struct CustomPush {
      uint32_t brdfLut;
      uint32_t sampleCount;
      float invSize;
      float padding[kPushConstantFloatSize - 3];
    } customPush{
        .brdfLut = index,
        .sampleCount = 1024,
        .invSize = invSize,
    };

    cmd.BindPipeline(m_pipelineHandle);
    cmd.PushConstants(rhi::EShaderStage::Compute, 0, sizeof(CustomPush),
                      &customPush);
    cmd.Dispatch(m_desc.extent.width / 8, m_desc.extent.height / 8, 1);
  }

private:
  ShaderHandle m_shader;

  const VirtualTextureDesc &m_desc;

  VirtualResourceHandle m_outputHandle;
  PipelineHandle m_pipelineHandle;
};

} // namespace avalon::graphics
