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

export namespace avalon::graphics {

class OpaquePass final : public RenderPass<OpaquePass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    m_colorHandle = builder.SetClearValue(ClearValue::Color(0, 0, 0.01))
                        .Write(kSwapchainColor, rhi::EResourceUsage::Present);
    m_depthHandle =
        builder.SetClearValue(ClearValue::DepthStencil(0, 0))
            .Write(kSceneDepth, rhi::EResourceUsage::DepthStencilAttachment);
  }

  void OnCompile(rhi::IRhi &rhi) override {}

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &packet = context.renderPacket;
    if (packet.opaqueBatches.IsEmpty())
      return;

    auto &materialManager = GetMaterialManager();
    auto &meshManager = GetMeshManager();

    rhi::BufferHandle lastVBO;
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

        auto currentVBO = mesh->GetVBO();
        auto currentIBO = mesh->GetIBO();

        if (currentVBO != lastVBO) {
          cmd.BindVertexBuffer(0, 1, &currentVBO, 0);
          lastVBO = currentVBO;
        }
        if (currentIBO != lastIBO) {
          cmd.BindIndexBuffer(currentIBO, 0, mesh->GetIndexFormat());
          lastIBO = currentIBO;
        }

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
};

} // namespace avalon::graphics
