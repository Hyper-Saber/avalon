module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
export module avalon.graphics:renderer;

import avalon.core;
import avalon.rhi;
import avalon.ecs;
import :render_pass;
import :opaque_pass;
import :mesh_extractor;
import :types;

export namespace avalon::graphics {
class AVALON_GRAPHICS_API Renderer : public NonCopyable,
                                     public mem::AutoDestroyable<Renderer> {

public:
  Renderer(rhi::IRhi &rhi, PipelineHandle handle)
      : m_rhi(rhi), m_basePipeline(handle) {}

  bool Initialize() {
    auto alignment =
        m_rhi.GetCapabilities().limits.minUniformBufferOffsetAlignment;
    m_globalUboStride = mem::AlignUp(sizeof(SceneGlobals), alignment);
    size_t totalSize = m_globalUboStride * m_rhi.GetMaxFrameInFlight();

    rhi::BufferCreateInfo info{
        .size = totalSize,
        .usage = rhi::EBufferUsage::Uniform,
        .memoryProperty = rhi::EMemoryProperty::DeviceLocal |
                          rhi::EMemoryProperty::HostVisible |
                          rhi::EMemoryProperty::HostCoherent,
    };

    m_globalUbo = m_rhi.CreateBuffer(info);
    m_globalUboMappedPtr = static_cast<uint8_t *>(m_rhi.MapMemory(m_globalUbo));

    return m_globalUbo.IsValid() && m_globalUboMappedPtr;
  }

  void SetClearColor(Color color) {
    for (auto &pass : m_passes) {
      pass->SetClearColor(color);
    }
  }

  void OnResize(const rhi::Extent2D &extent) {
    for (auto &pass : m_passes) {
      pass->OnResize(extent);
    }
  }

  void AddPass(UniquePtr<IRenderPass> pass) {
    m_passes.PushBack(std::move(pass));
  }

  void Render(rhi::ICommandBuffer &cmd, ecs::World &world,
              const SceneGlobals &globals) {

    RenderContext context{
        .cmd = cmd,
    };

    auto frameIndex = m_rhi.GetCurrentFrameIndex();
    auto offset = frameIndex * m_globalUboStride;

    std::memcpy(m_globalUboMappedPtr + offset, &globals, sizeof(SceneGlobals));

    auto writer = m_rhi.CreateDescriptorWriter(m_basePipeline, 0);
    if (writer->IsValid()) {
      auto set = writer
                     ->WriteBuffer("uSceneGlobals"_id,
                                   {
                                       .buffer = m_globalUbo,
                                       .offset = offset,
                                       .range = sizeof(SceneGlobals),
                                   })
                     .Build();

      if (!set.IsValid()) {
        return;
      }
      context.globalSet = set;
    }

    m_packet.Clear();
    m_extractor.Extract(world, m_packet);

    for (auto &pass : m_passes) {
      pass->Execute(context, m_packet);
    }
  }

private:
  rhi::IRhi &m_rhi;
  MeshExtractor m_extractor;
  RenderPacket m_packet;
  rhi::PipelineHandle m_basePipeline;
  Array<UniquePtr<IRenderPass>> m_passes;

  size_t m_globalUboStride;
  rhi::BufferHandle m_globalUbo;
  uint8_t *m_globalUboMappedPtr;
};
} // namespace avalon::graphics
