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
  }

  void OnCompile(rhi::IRhi &rhi) override {}

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &packet = context.renderPacket;
    if (packet.opaqueBatches.IsEmpty())
      return;

    auto &materialManager = GetMaterialManager();
    auto &meshManager = GetMeshManager();

    rhi::BufferHandle lastPosVBO;
    rhi::BufferHandle lastAttriVBO;
    rhi::BufferHandle lastIBO;
    rhi::PipelineHandle lastPipeline;

    for (auto &batch : packet.opaqueBatches) {
      auto &material = *materialManager.Resolve(batch.material);
      auto pipeline = material.GetOrCreatePipeline(
          context.rhi, context.pipelineRenderingInfo);

      if (pipeline != lastPipeline) {
        cmd.BindPipeline(pipeline);
        lastPipeline = pipeline;
      }

      for (uint32_t i = batch.firstInstance;
           i < batch.firstInstance + batch.instanceCount; i++) {
        auto *mesh = meshManager.Resolve(packet.meshHandles[i]);
        if (!mesh) [[unlikely]]
          continue;

        auto currentPosVBO = mesh->GetPosVBO();
        auto currentAttriVBO = mesh->GetAttriVBO();
        auto currentIBO = mesh->GetIBO();

        if (currentPosVBO != lastPosVBO || currentAttriVBO != lastAttriVBO) {
          BufferHandle buffers[] = {currentPosVBO, currentAttriVBO};
          cmd.BindVertexBuffer(0, 2, buffers, 0);
          lastPosVBO = currentPosVBO;
          lastAttriVBO = currentAttriVBO;
        }
        if (currentIBO != lastIBO) {
          cmd.BindIndexBuffer(currentIBO, 0, mesh->GetIndexFormat());
          lastIBO = currentIBO;
        }

        auto prefilteredTexture =
            context.GetPhysicalTexture(m_prefilteredHandle);
        auto allocation = context.GetPhysicalBufferAllocation(m_skyboxSHHandle);

        auto &bindlessManager = context.rhi.GetBindlessManager();
        auto prefilteredIndex =
            bindlessManager.RegisterTextureCube(prefilteredTexture);
        auto brdfLutIndex = bindlessManager.RegisterTexture(
            context.GetPhysicalTexture(m_brdfLudHandle));

        packet.pushConstants[i].customSlots[kSkyboxPrefilteredSlot] =
            prefilteredIndex;
        packet.pushConstants[i].customSlots[kBRDFLutSlot] = brdfLutIndex;
        packet.pushConstants[i].customSlots[kSkyboxSHSlot] = allocation.offset;

        // Debug("begin opaque pass, buffer: {}, offset: {}, size: {}",
        // allocation.buffer.id, allocation.offset, allocation.size);
        cmd.PushConstants(rhi::EShaderStage::All, 0,
                          sizeof(StandardPushConstant),
                          &packet.pushConstants[i]);

        cmd.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
      }
    }
  }

private:
  VirtualResourceHandle m_colorHandle;
  VirtualResourceHandle m_depthHandle;
  VirtualResourceHandle m_skyboxSHHandle;
  VirtualResourceHandle m_prefilteredHandle;
  VirtualResourceHandle m_brdfLudHandle;

  MaterialInstance *m_materialInstance;
};

} // namespace avalon::graphics
