module;
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:render_context;

import avalon.core;
import avalon.rhi;
import avalon.ecs;
import :renderer_types;
import :virtual_resource_manager;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API RenderContext final
    : public NonCopyable,
      public mem::AutoDestroyable<RenderContext>,
      public ecs::IRenderDataSink {
public:
  explicit RenderContext(rhi::IRhi &rhi, VirtualResourceManager &manager,
                         RenderPacket &packet)
      : rhi(rhi), m_resourceManager(manager), renderPacket(packet) {}

  rhi::IRhi &rhi;
  RenderPacket &renderPacket;
  PipelineRenderingInfo pipelineRenderingInfo;
  rhi::ICommandBuffer *cmd{nullptr};
  rhi::DescriptorSetHandle sceneGlobalsSet;
  uint32_t sceneGlobalsSetDynamicOffset;

  auto GetPhysicalTexture(VirtualResourceHandle handle) const
      -> rhi::TextureHandle {

    return m_resourceManager.GetPhysicalTexture(handle);
  }

  auto GetPhysicalBufferAllocation(VirtualResourceHandle handle) const
      -> rhi::BufferAllocation {
    auto alloc = m_resourceManager.GetPhysicalBufferAllocation(handle);
    AVALON_ASSERT(alloc.has_value());
    return alloc.value();
  }

private:
  VirtualResourceManager &m_resourceManager;
};

} // namespace avalon::graphics
