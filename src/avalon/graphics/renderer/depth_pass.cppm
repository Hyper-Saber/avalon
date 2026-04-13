module;
export module avalon.graphics:depth_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class DepthPass final : public RenderPass<DepthPass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    builder.SetClearValue(ClearValue::DepthStencil(0, 0))
        .WriteAttachment(kSceneDepth, EResourceUsage::DepthStencilAttachment);
    builder.SetClearValue(ClearValue::Black())
        .SetFormat(EFormat::R16_SFLOAT)
        .WriteAttachment("InstanceID"_id, EResourceUsage::ColorAttachment);
  }

  void OnCompile(rhi::IRhi &rhi) override {}

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &packet = context.renderPacket;
    if (packet.opaqueBatches.IsEmpty())
      return;

    auto &materialManager = GetMaterialManager();

    auto materialHandle = materialManager.TryGetMaterial("Depth"_id);
    auto pMat = materialManager.Resolve(materialHandle);

    auto pipeline =
        pMat->GetOrCreatePipeline(context.rhi, context.pipelineRenderingInfo);

    auto indirectAlloc = packet.indirectCommandBufferAllocation;

    StandardPushConstants push{
        .instanceBufferOffset = packet.opaqueInstanceDataBaseOffset,
    };

    cmd.BindIndexBuffer(context.rhi.GetIndexBuffer(), 0, EFormat::R32_Uint);

    cmd.BindPipeline(pipeline);
    cmd.PushConstants(rhi::EShaderStage::All, 0, sizeof(StandardPushConstants),
                      &push);

    cmd.DrawIndexedIndirect(indirectAlloc.buffer, indirectAlloc.offset,
                            packet.totalCommandCount,
                            sizeof(IndexedIndirectCommand));
  }

private:
};
} // namespace avalon::graphics
