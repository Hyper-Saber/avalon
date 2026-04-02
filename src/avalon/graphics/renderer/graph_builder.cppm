module;
#include <optional>
#include <utility>
export module avalon.graphics:render_graph_builder;

import avalon.core;
import avalon.rhi;
import :renderer_types;
import :render_pass;
import :render_graph;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API RenderGraphBuilder final : public NonCopyable {
public:
  explicit RenderGraphBuilder(class RenderGraph &graph)
      : m_graph(graph), m_owner(nullptr), m_rhi(&graph.m_rhi) {}

  RenderGraphBuilder(class RenderGraph &graph, IRenderPass &owner,
                     rhi::IRhi *rhi)
      : m_graph(graph), m_owner(&owner), m_rhi(rhi) {}

  template <TRenderPass T, typename... Args>
  T &AddPass(StringId name, Args &&...args) {
    auto &pass = m_graph.AddPass<T>(name, std::forward<Args>(args)...);
    RenderGraphBuilder setupBuilder(m_graph, pass, m_rhi);
    pass.Setup(setupBuilder);
    return pass;
  }

  auto Write(StringId name,
             rhi::EResourceUsage usage = rhi::EResourceUsage::ColorAttachment)
      -> VirtualResourceHandle;
  auto Read(StringId name,
            rhi::EResourceUsage usage = rhi::EResourceUsage::ReadOnly)
      -> VirtualResourceHandle;

  auto SetExtent(const rhi::Extent2D &extent) -> RenderGraphBuilder &;
  auto SetRelativeExtent(float scale = 1.f) -> RenderGraphBuilder &;
  auto SetFormat(rhi::EFormat format) -> RenderGraphBuilder &;
  auto SetSamplerCount(rhi::ESampleCount count) -> RenderGraphBuilder &;

  auto SetLoadOp(rhi::EAttachmentLoadOp op) -> RenderGraphBuilder &;
  auto SetStoreOp(rhi::EAttachmentStoreOp op) -> RenderGraphBuilder &;
  auto SetClearValue(rhi::ClearValue value) -> RenderGraphBuilder &;

private:
  rhi::EFormat SpecifyTextureDefaultFormat(rhi::EResourceUsage usage);

  rhi::IRhi *m_rhi;
  class RenderGraph &m_graph;
  IRenderPass *m_owner;
  VirtualTextureDesc m_desc{};
  std::optional<rhi::EAttachmentLoadOp> m_loadOp;
  std::optional<rhi::EAttachmentStoreOp> m_storeOp;
  std::optional<ClearValue> m_clearValue;

  VirtualResourceHandle m_currentResource;
};

} // namespace avalon::graphics
