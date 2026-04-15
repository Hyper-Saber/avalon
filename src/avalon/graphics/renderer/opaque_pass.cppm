module;
#include <cstdint>
export module avalon.graphics:opaque_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

class OpaquePass final : public RenderPass<OpaquePass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    m_colorHandle =
        builder.SetClearValue(ClearValue::Color(0, 0, 0.01))
            .WriteAttachment(kSceneColor, rhi::EResourceUsage::ColorAttachment);
    m_depthHandle =
        builder.SetLoadOp(rhi::EAttachmentLoadOp::Load)
            .WriteAttachment(kSceneDepth,
                             rhi::EResourceUsage::DepthStencilAttachment |
                                 rhi::EResourceUsage::ReadOnly);
    m_prefilteredHandle =
        builder.Read("SkyboxPrefiltered"_id, EResourceUsage::ReadOnly,
                     EResourceUsage::ReadOnly, EShaderStage::Fragment);
    m_skyboxSHHandle = builder.ReadBuffer(
        "SkyboxSH"_id, EResourceUsage::ReadOnly, EShaderStage::Fragment);
    m_brdfLudHandle =
        builder.Read("BRDFLut"_id, EResourceUsage::ReadOnly,
                     EResourceUsage::ReadOnly, EShaderStage::Fragment);
    m_shdowMaskHandle =
        builder.Read("ShadowMask"_id, EResourceUsage::ReadOnly,
                     EResourceUsage::ReadOnly, EShaderStage::Fragment);
  }

  void OnCompile(rhi::IRhi &rhi) override {}

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &packet = context.renderPacket;
    if (packet.opaqueBatches.IsEmpty())
      return;

    auto &rhi = context.rhi;
    auto &materialManager = GetMaterialManager();
    auto &bindlessManager = rhi.GetBindlessManager();

    uint32_t prefilteredIndex = bindlessManager.RegisterTextureCube(
        context.GetPhysicalTexture(m_prefilteredHandle));
    uint32_t brdfLutIndex = bindlessManager.RegisterTexture(
        context.GetPhysicalTexture(m_brdfLudHandle));
    auto shAllocation = context.GetPhysicalBufferAllocation(m_skyboxSHHandle);
    uint32_t shadowMaskIndex = bindlessManager.RegisterTexture(
        context.GetPhysicalTexture(m_shdowMaskHandle));

    StandardPushConstants passData{
        .instanceBufferOffset = packet.opaqueInstanceDataBaseOffset,
        .skyboxSHOffset = shAllocation.offset,
        .prefilterMap = prefilteredIndex,
        .brdfLut = brdfLutIndex,
        .shadowMask = shadowMaskIndex,
    };

    cmd.BindIndexBuffer(rhi.GetIndexBuffer(), 0, EFormat::R32_Uint);

    auto indirectAlloc = packet.indirectCommandBufferAllocation;

    for (auto &batch : packet.opaqueBatches) {
      auto *material = materialManager.Resolve(batch.material);
      if (!material)
        continue;

      auto pipeline =
          material->GetOrCreatePipeline(rhi, context.pipelineRenderingInfo);
      cmd.BindPipeline(pipeline);

      cmd.PushConstants(rhi::EShaderStage::All, 0,
                        sizeof(StandardPushConstants), &passData);

      cmd.DrawIndexedIndirect(
          indirectAlloc.buffer,
          indirectAlloc.offset +
              (batch.commandOffset * sizeof(IndexedIndirectCommand)),
          batch.commandCount, sizeof(IndexedIndirectCommand));
    }
  }

private:
  VirtualResourceHandle m_colorHandle;
  VirtualResourceHandle m_depthHandle;
  VirtualResourceHandle m_skyboxSHHandle;
  VirtualResourceHandle m_prefilteredHandle;
  VirtualResourceHandle m_brdfLudHandle;
  VirtualResourceHandle m_shdowMaskHandle;

  MaterialInstance *m_materialInstance;
};

} // namespace avalon::graphics
