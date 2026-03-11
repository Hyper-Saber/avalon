module;
#include <utility>
export module avalon.graphics:renderer;

import avalon.core;
import avalon.rhi;
import avalon.ecs;
import :render_pass;
import :opaque_pass;
import :mesh_extractor;

export namespace avalon::graphics {
class AVALON_GRAPHICS_API Renderer : public NonCopyable,
                                     public mem::AutoDestroyable<Renderer> {
public:
  Renderer(rhi::IRhi &rhi) : m_rhi(rhi) {}

  void OnResize(const rhi::Extent2D &extent) {
    for (auto &pass : m_passes) {
      pass->OnResize(extent);
    }
  }

  void AddPass(UniquePtr<IRenderPass> pass) {
    m_passes.PushBack(std::move(pass));
  }

  void Render(rhi::ICommandBuffer &cmd, ecs::World &world) {
    m_extractor.Extract(world, m_packet);
    RenderContext context{.cmd = cmd};

    for (auto &pass : m_passes) {
      pass->Execute(context, m_packet);
    }
  }

private:
  rhi::IRhi &m_rhi;
  MeshExtractor m_extractor;
  RenderPacket m_packet;
  Array<UniquePtr<IRenderPass>> m_passes;
};
} // namespace avalon::graphics
