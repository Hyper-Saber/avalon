module;
#include <cstdint>
export module avalon.graphics:mesh_render_executor;

import avalon.core;
import avalon.rhi;
import :render_packet_extractor;
import :mesh;
import :utils;
import :render_pass;
import :mesh_manager;

export namespace avalon::graphics {

class MeshRenderExecutor {
public:
  MeshRenderExecutor() = default;
  MeshRenderExecutor(rhi::EShaderStage pushConstantStageMask)
      : m_pushConstantStageMask(pushConstantStageMask) {}

  void Execute(ICommandBuffer &cmd, const RenderPacket &packet) {
    if (packet.IsEmpty())
      return;

    rhi::BufferHandle lastVBO;
    rhi::BufferHandle lastIBO;

    for (auto &batch : packet.batches) {
      for (uint32_t i = batch.firstInstance; i < batch.instanceCount; i++) {
        auto mesh = GetMeshManager().Resolve(packet.meshHandles[i]);
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

        if (m_pushConstantStageMask != rhi::EShaderStage::None)
          cmd.PushConstants(m_pushConstantStageMask, 0, sizeof(PushConstant),
                            &packet.pushConstants[i]);

        if (batch.materialSet.IsValid()) {
          auto &offsets = packet.materialOffsets[i];
          cmd.BindDescriptorSet(1, {&batch.materialSet, 1},
                                {offsets.GetData(), offsets.GetSize()});
        }

        cmd.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
      }
    }
  }

private:
  rhi::EShaderStage m_pushConstantStageMask;
};

} // namespace avalon::graphics
