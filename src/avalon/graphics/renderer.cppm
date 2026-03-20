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
import :render_packet_extractor;
import :types;

namespace {
constexpr size_t kSegmentSize = 1024 * 1024 * 16;
};

export namespace avalon::graphics {
class AVALON_GRAPHICS_API Renderer : public NonCopyable,
                                     public mem::AutoDestroyable<Renderer> {

public:
  Renderer(rhi::IRhi &rhi, PipelineHandle handle)
      : m_rhi(rhi), m_basePipeline(handle) {}

  ~Renderer() { m_uboPool.Reset(); }

  bool Initialize() {
    m_uboPool = MakeUnique<rhi::RingBufferPool>(
        m_rhi, EBufferUsage::Uniform, EMemoryProperty::All, kSegmentSize);

    return m_uboPool.Get() != nullptr;
  }

  void SetClearColor(Color color) {
    for (auto &pass : m_passes) {
      pass->SetClearColor(color);
    }
  }

  void OnResize(const rhi::Extent2D &extent) {
    m_resolution.width = static_cast<float>(extent.width);
    m_resolution.height = static_cast<float>(extent.height);
    m_resolution.invWidth = 1.0f / extent.width;
    m_resolution.invHeight = 1.0f / extent.height;

    for (auto &pass : m_passes) {
      pass->OnResize(extent);
    }
  }

  void AddPass(UniquePtr<IRenderPass> pass) {
    m_passes.PushBack(std::move(pass));
  }

  void Render(rhi::ICommandBuffer &cmd, ecs::World &world,
              SceneGlobals &globals) {

    m_uboPool->ResetPool();

    globals.time = GetContext().globalTime;
    globals.resolution = m_resolution;

    RenderContext context{
        .rhi = m_rhi,
        .cmd = cmd,
        .uboHandle = m_uboPool->GetBufferHandle(),
    };

    auto &writer = m_rhi.CreateDescriptorWriter(m_basePipeline, 0);
    if (writer.IsValid()) {
      auto allocation = m_uboPool->AllocateAligned(sizeof(SceneGlobals));
      std::memcpy(allocation.pHostAddress, &globals, sizeof(SceneGlobals));
      auto set = writer
                     .WriteBuffer("uSceneGlobals"_id,
                                  {
                                      .buffer = allocation.buffer,
                                      .offset = allocation.offset,
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

    uint32_t currentBatchStart = 0;
    uint32_t index = 0;
    uint32_t total = m_packet.materialInstances.GetSize();
    MaterialHandle currentBatchMaterial;
    for (auto handle : m_packet.materialInstances) {
      auto materialInstance = GetMaterialManager().Resolve(handle);
      auto &dataBlob = materialInstance->GetDataBlob();
      auto allocation = m_uboPool->AllocateAligned(dataBlob.GetSize());
      std::memcpy(allocation.pHostAddress, dataBlob.GetData(),
                  dataBlob.GetSize());
      auto &buffers = materialInstance->GetBufferStates();

      Array<uint32_t> offsets = Array<uint32_t>(buffers.GetSize());
      for (auto &buffer : buffers) {
        offsets[buffer.bindingPoint] = allocation.offset;
      }

      m_packet.materialOffsets.PushBack(std::move(offsets));

      auto material = materialInstance->GetMaterialHandle();
      if (!currentBatchMaterial.IsValid()) {
        currentBatchMaterial = material;
      }
      bool isNewBatch = material != currentBatchMaterial || index == total - 1;
      if (isNewBatch) {
        RenderBatch batch{
            .firstInstance = currentBatchStart,
            .instanceCount = index - currentBatchStart,
        };
        currentBatchStart = index;
        currentBatchMaterial = material;
        m_packet.batches.PushBack(std::move(batch));
      }
      index++;
    }

    if (m_packet.batches.GetSize() > 0)
      m_packet.batches.GetBack().instanceCount++;

    for (auto &pass : m_passes) {
      pass->Execute(context, m_packet);
    }
  }

private:
  Resolution m_resolution;
  rhi::IRhi &m_rhi;
  RenderPacketExtractor m_extractor;
  RenderPacket m_packet;
  rhi::PipelineHandle m_basePipeline;
  Array<UniquePtr<IRenderPass>> m_passes;
  UniquePtr<rhi::RingBufferPool> m_uboPool;
};
} // namespace avalon::graphics
