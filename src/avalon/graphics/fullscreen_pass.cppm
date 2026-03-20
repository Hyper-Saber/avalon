module;
export module avalon.graphics:fullscreen_pass;

import avalon.core;
import avalon.shader;
import :render_pass;
import :types;

namespace {
constexpr avalon::StringView kFullscreenPassName = "FullscreenPass";
}

export namespace avalon::graphics {

class AVALON_GRAPHICS_API FullscreenPass final : public RenderPass<FullscreenPass> {
public:
  FullscreenPass(rhi::PipelineHandle pipeline, rhi::RenderPassHandle renderPass,
             ShaderHandle handle, rhi::Extent2D extent)
      : m_pipeline(pipeline), m_rendePass(renderPass), m_extent(extent) {}

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

    if (context.globalSet.IsValid())
      context.cmd.BindDescriptorSet(0, {&context.globalSet, 1}, {});

    context.cmd.Draw(3, 1, 0, 0);
    context.cmd.EndRenderPass();
  }

  constexpr StringView GetName() const override { return kFullscreenPassName; }

private:
  void Setup(rhi::RenderPassBeginInfo &outInfo) override {
    outInfo.renderPassHandle = m_rendePass;
    outInfo.renderTarget = rhi::ERenderTarget::SwapchainBackBuffer;

    outInfo.renderArea.offset = {0, 0};
    outInfo.renderArea.extent = m_extent;

    outInfo.clearValues.Clear();
    outInfo.clearValues.PushBack(
        rhi::ClearValue::Color(m_color.r, m_color.g, m_color.b));
    outInfo.clearValues.PushBack(rhi::ClearValue::DepthStencil());
  }

  rhi::PipelineHandle m_pipeline;
  rhi::RenderPassHandle m_rendePass;
  Extent2D m_extent;
  Color m_color = {0, 0, 0.01f, 1};
};
} // namespace avalon::graphics
