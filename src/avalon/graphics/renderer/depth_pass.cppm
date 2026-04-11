module;
#include <cstdint>
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
  }

  void OnCompile(rhi::IRhi &rhi) override {}

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto &packet = context.renderPacket;
    if (packet.opaqueBatches.IsEmpty())
      return;

    auto &materialManager = GetMaterialManager();
    auto materialHandle = materialManager.TryGetMaterial("Depth"_id);
    auto pMat = materialManager.Resolve(materialHandle);

    cmd.BindPipeline(
        pMat->GetOrCreatePipeline(context.rhi, context.pipelineRenderingInfo));
    rhi::BufferHandle lastVBO;
    rhi::BufferHandle lastIBO;

    auto &meshManager = GetMeshManager();

    for (auto &batch : packet.opaqueBatches) {
      for (uint32_t i = batch.firstInstance;
           i < batch.firstInstance + batch.instanceCount; i++) {
        auto *mesh = meshManager.Resolve(packet.meshHandles[i]);
        if (!mesh) [[unlikely]]
          continue;

        auto currentVBO = mesh->GetPosVBO();
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
};
} // namespace avalon::graphics
