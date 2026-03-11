module;
#include <cstddef>
#include <cstring>
export module avalon.graphics:mesh_render_executor;

import avalon.core;
import avalon.rhi;
import :mesh_extractor;
import :mesh;
import :utils;
import :mesh_manager;

export namespace avalon::graphics {

class MeshRenderExecutor {
public:
  void Execute(rhi::ICommandBuffer &cmd, rhi::PipelineHandle pipeline,
               const RenderPacket &packet) {
    if (packet.IsEmpty())
      return;

    rhi::BufferHandle lastVBO;
    rhi::BufferHandle lastIBO;

    cmd.BindPipeline(pipeline);
    const size_t instanceCount = packet.meshHandles.GetSize();
    for (size_t i = 0; i < instanceCount; i++) {
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

      cmd.PushConstants(rhi::EShaderStage::Vertex, 0, sizeof(Matrix4x4),
                        &packet.transforms[i]);

      cmd.DrawIndexed(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
  }
};

} // namespace avalon::graphics
