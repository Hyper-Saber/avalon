module;
export module avalon.graphics:opaque_pass;

import avalon.core;
import avalon.shader;
import :mesh_render_executor;
import :render_pass;
import :types;

namespace {
constexpr avalon::StringView kOpaquePassName = "OpaquePass";
}

export namespace avalon::graphics {

class AVALON_GRAPHICS_API OpaquePass final : public RenderPass<OpaquePass> {
public:
  OpaquePass(rhi::PipelineHandle pipeline, rhi::RenderPassHandle renderPass,
             ShaderHandle handle, rhi::Extent2D extent,
             rhi::RenderTargetBinding targets = {})
      : m_pipeline(pipeline), m_rendePass(renderPass), m_extent(extent),
        m_targets(targets) {
    auto shader = GetShaderManager().Resolve(handle);
    auto mask = shader->GetPushConstantStageMask();
    m_executor = MeshRenderExecutor(mask);
  }

  void SetClearColor(Color color) override { m_color = color; }

  void OnResize(const rhi::Extent2D &extent) override { m_extent = extent; }

  void Execute(RenderContext &context, RenderPacket &packet) override {
    RenderPassBeginInfo info;
    Setup(info);

    context.cmd.BeginRenderPass(info);
    rhi::Viewport viewport = {0,
                              0,
                              static_cast<float>(m_extent.width),
                              static_cast<float>(m_extent.height),
                              0,
                              1};

    context.cmd.SetViewport(viewport);
    context.cmd.SetScissor(info.renderArea);

    context.cmd.BindPipeline(m_pipeline);

    auto &writer = context.rhi.CreateDescriptorWriter(m_pipeline, 1);
    if (writer.IsValid()) {
      for (auto &batch : packet.batches) {
        auto material = GetMaterialManager().Resolve(
            packet.materialInstances[batch.firstInstance]);
        for (auto &buffer : material->GetBufferStates()) {
          BufferWriteInfo info{
              .buffer = context.uboHandle,
              .offset = buffer.bufferOffset,
              .range = buffer.size,
          };
          writer.WriteBuffer(buffer.nameHash, info);
        }
        batch.materialSet = writer.Build();
      }
    }
    if (context.globalSet.IsValid())
      context.cmd.BindDescriptorSet(0, {&context.globalSet, 1}, {});

    m_executor.Execute(context.cmd, packet);

    context.cmd.EndRenderPass();
  }

  constexpr StringView GetName() const override { return kOpaquePassName; }

private:
  void Setup(rhi::RenderPassBeginInfo &outInfo) override {
    outInfo.renderPassHandle = m_rendePass;
    outInfo.targets = m_targets;

    outInfo.renderArea.offset = {0, 0};
    outInfo.renderArea.extent = m_extent;

    outInfo.clearValues.Clear();
    outInfo.clearValues.PushBack(
        rhi::ClearValue::Color(m_color.r, m_color.g, m_color.b));
    outInfo.clearValues.PushBack(rhi::ClearValue::DepthStencil());
  }

  MeshRenderExecutor m_executor;

  rhi::PipelineHandle m_pipeline;
  rhi::RenderPassHandle m_rendePass;
  rhi::RenderTargetBinding m_targets;
  Extent2D m_extent;
  Color m_color = {0, 0, 0.01f, 1};
};
} // namespace avalon::graphics
