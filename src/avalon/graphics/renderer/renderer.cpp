module;
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
#include <numeric>

module avalon.graphics;

import :renderer;
import :render_graph;
import :render_context;
import :renderer_types;
import :render_pipeline;
import :virtual_resource_manager;
import :utils;
import avalon.rhi;
import avalon.core;
import :types;

namespace avalon::graphics {

class Renderer final : public IRenderer {
public:
  explicit Renderer(rhi::IRhi &rhi) : m_rhi(rhi) {
    m_resourceManager = MakeUnique<VirtualResourceManager>(rhi);
    m_context =
        MakeUnique<RenderContext>(m_rhi, *m_resourceManager.Get(), m_packet);

    auto &writer = m_rhi.GetSceneGlobalSetWriter();
    if (writer.IsValid()) {
      rhi::BufferWriteInfo info{
          .buffer = m_rhi.GetUBOPool().GetBufferHandle(),
          .offset = 0,
          .range = sizeof(SceneGlobals),
      };
      m_context->sceneGlobalsSet =
          writer.WriteBuffer(kSceneGlobalsBuffer, info).Build();
    }
    AVALON_ASSERT(m_context->sceneGlobalsSet.IsValid());
  }

  void Destroy() override {
    this->~Renderer();
    mem::Allocator<Renderer> alloc;
    alloc.Deallocate(this, 1);
  }

  void OnResize(uint32_t width, uint32_t height) override {
    if (width == 0 || height == 0)
      return;

    m_rhi.WaitIdle();

    m_resolution.width = static_cast<float>(width);
    m_resolution.height = static_cast<float>(height);
    m_resolution.invWidth = 1.0f / m_resolution.width;
    m_resolution.invHeight = 1.0f / m_resolution.height;

    m_rhi.RecreateSwapchain(width, height);
  }

  void Render(const SceneSnapshot &snapshot) override {
    if (!m_pipeline)
      return;

    m_packet.Clear();
    m_resourceManager->ResetPool();

    auto cmd = m_rhi.GetMainCommandBuffer();
    m_context->cmd = cmd;
    cmd->Begin();
    GetMaterialManager().Update(m_rhi);

    SceneGlobals globals{
        .camera = snapshot.camera,
        .lightData = snapshot.lightData,
        .time = GetContext().globalTime,
        .resolution = m_resolution,
    };
    UpdateSceneGlobalSet(*cmd, globals);

    PrepareMaterialBatches(m_packet, snapshot);

    RenderGraph graph(m_rhi, *m_resourceManager.Get());

    VirtualTextureDesc desc{
        .nameHash = kSwapchainColor,
        .usage = EResourceUsage::Present,
        .format = m_rhi.GetSwapchainImageFormat(),
        .extent = m_rhi.GetSwapchainExtent(),
    };

    auto presentTexture = m_rhi.GetCurrentPresentTexture();
    graph.ImportExternalTexture(kSwapchainColor, presentTexture, desc);

    RenderGraphBuilder builder(graph);
    m_pipeline->Setup(builder, m_packet);

    graph.Compile();
    graph.Render(*cmd, *m_context);
    cmd->End();
    m_rhi.Submit(*cmd);
  }

  void SetPipeline(IRenderPipeline *pipeline) override {
    m_pipeline = pipeline;
  }

  rhi::IRhi &GetRhi() override { return m_rhi; }

private:
  void UpdateSceneGlobalSet(rhi::ICommandBuffer &cmd,
                            const SceneGlobals &globals) {
    auto allocation = m_rhi.GetUBOPool().AllocateAligned(sizeof(SceneGlobals));
    std::memcpy(allocation.pHostAddress, &globals, sizeof(SceneGlobals));
    m_context->sceneGlobalsSetDynamicOffset = allocation.offset;
  }

  void PrepareMaterialBatches(RenderPacket &packet,
                              const SceneSnapshot &snapshot) {
    uint32_t totalInstances = snapshot.opaqueMeshHandles.GetSize();
    if (totalInstances == 0)
      return;

    auto &materialManager = GetMaterialManager();

    Array<uint32_t> indices(totalInstances);
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&](uint32_t a, uint32_t b) {
      auto matA = snapshot.opaqueMaterials[a];
      auto matB = snapshot.opaqueMaterials[b];
      if (matA != matB)
        return matA < matB;
      return snapshot.opaqueMeshHandles[a] < snapshot.opaqueMeshHandles[b];
    });

    auto lastMaterial = ResourceHandle::Invalid();

    for (uint32_t i : indices) {
      auto currInstHandle = snapshot.opaqueMaterialInstances[i];
      auto currMaterialHandle = snapshot.opaqueMaterials[i];

      auto &materialInstance = materialManager.ResolveSafe({currInstHandle.id});
      uint32_t materialGpuIndex = materialInstance.GetGpuIndex();

      const Matrix4x4 &worldMatrix = snapshot.opaqueWorldMatrices[i];
      StandardPushConstant pc{.model = worldMatrix,
                              .normalMatrix = ComputeNormalMatrix(worldMatrix),
                              .materialIndex = materialGpuIndex};
      auto textureSlots = materialInstance.GetTextureSlots();
      AVALON_ASSERT(textureSlots.GetSize() <= kMaxCustomSlots);
      std::memcpy(pc.customSlots, textureSlots.GetData(),
                  textureSlots.GetSize());

      packet.pushConstants.PushBack(pc);

      if (currMaterialHandle != lastMaterial) {
        RenderBatch batch;
        batch.material = {currMaterialHandle.id};
        batch.firstInstance = packet.pushConstants.GetSize() - 1;
        batch.instanceCount = 0;

        packet.opaqueBatches.PushBack(batch);
        lastMaterial = currMaterialHandle;
      }

      packet.opaqueBatches.GetBack().instanceCount++;

      packet.meshHandles.PushBack({snapshot.opaqueMeshHandles[i].id});
    }
  }

  rhi::IRhi &m_rhi;
  IRenderPipeline *m_pipeline;
  UniquePtr<RenderContext> m_context;
  UniquePtr<VirtualResourceManager> m_resourceManager;

  RenderPacket m_packet;
  Resolution m_resolution;
};

auto CreateRenderer(rhi::IRhi &rhi) -> UniquePtr<IRenderer> {
  return MakeUnique<Renderer>(rhi);
}

} // namespace avalon::graphics
