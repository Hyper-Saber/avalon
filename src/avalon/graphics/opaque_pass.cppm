module;
export module avalon.graphics:opaque_pass;

import avalon.core;
import :mesh_render_executor;
import :render_pass;

export namespace avalon::graphics {

constexpr StringView kOpaquePassName = "OpaquePass";

class AVALON_GRAPHICS_API OpaquePass final : public RenderPass<OpaquePass> {
public:
  OpaquePass(rhi::PipelineHandle pipeline, rhi::RenderPassHandle renderPass,
             rhi::Extent2D extent)
      : m_pipeline(pipeline), m_rendePass(renderPass), m_extent(extent) {}

  void SetClearColor(Color color) override { m_color = color; }

  void OnResize(const rhi::Extent2D &extent) override { m_extent = extent; }

  void Execute(RenderContext &context, const RenderPacket &packet) override {
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

    m_executor.Execute(context.cmd, m_pipeline, packet);

    context.cmd.EndRenderPass();
  }

  constexpr StringView GetName() const override { return kOpaquePassName; }

private:
  void Setup(rhi::RenderPassBeginInfo &outInfo) override {
    outInfo.renderPassHandle = m_rendePass;
    outInfo.renderTarget = rhi::ERenderTarget::SwapchainBackBuffer;

    outInfo.renderArea.offset = {0, 0};
    outInfo.renderArea.extent = m_extent;

    outInfo.clearValues.Clear();
    outInfo.clearValues.PushBack(
        rhi::ClearValue::Color(m_color.r, m_color.g, m_color.b));
    // outInfo.clearValues.PushBack(rhi::ClearValue::DepthStencil());
  }

  MeshRenderExecutor m_executor;

  rhi::PipelineHandle m_pipeline;
  rhi::RenderPassHandle m_rendePass;
  Extent2D m_extent;
  Color m_color;
};
} // namespace avalon::graphics
