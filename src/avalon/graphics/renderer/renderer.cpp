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
    m_context = MakeUnique<RenderContext>(m_rhi, *m_resourceManager.Get(),
                                          m_packet, m_resolution);

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

    m_resolution.width = width;
    m_resolution.height = height;
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
    m_resourceManager->ImportExternalBuffer("SceneGlobals"_id, allocation);
  }

  void PrepareMaterialBatches(RenderPacket &packet,
                              const SceneSnapshot &snapshot) {
    uint32_t totalInstances = snapshot.opaqueMeshHandles.GetSize();
    AVALON_ASSERT(snapshot.opaqueWorldMatrices.GetSize() == totalInstances);
    AVALON_ASSERT(snapshot.opaqueInvWorldMatrices.GetSize() == totalInstances);

    if (totalInstances == 0)
      return;

    auto &ssboPool = m_rhi.GetDynamicSSBOPool();

    auto modelAlloc =
        ssboPool.AllocateAligned(totalInstances * sizeof(Matrix4x4));
    auto invModelAlloc =
        ssboPool.AllocateAligned(totalInstances * sizeof(Matrix4x4));

    std::memcpy(modelAlloc.pHostAddress, snapshot.opaqueWorldMatrices.GetData(),
                modelAlloc.size);
    std::memcpy(invModelAlloc.pHostAddress,
                snapshot.opaqueInvWorldMatrices.GetData(), invModelAlloc.size);

    Array<uint32_t> indices(totalInstances);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](uint32_t a, uint32_t b) {
      if (snapshot.opaqueMaterials[a] != snapshot.opaqueMaterials[b])
        return snapshot.opaqueMaterials[a] < snapshot.opaqueMaterials[b];
      return snapshot.opaqueMeshHandles[a] < snapshot.opaqueMeshHandles[b];
    });

    auto lastMaterial = ResourceHandle::Invalid();
    auto lastMesh = ResourceHandle::Invalid();

    auto matrixSize = static_cast<uint32_t>(sizeof(Matrix4x4));

    Array<InstanceData> instanceDatas;
    Array<IndexedIndirectCommand> indirectCommands;

    instanceDatas.Reserve(totalInstances);
    indirectCommands.Reserve(totalInstances / 2);

    for (uint32_t i : indices) {
      uint32_t offset = i * matrixSize;
      uint32_t modelOffset = modelAlloc.offset + offset;
      uint32_t invOffset = invModelAlloc.offset + offset;

      auto gpuData = BuildInstanceData(i, snapshot, modelOffset, invOffset);

      instanceDatas.PushBack(gpuData);

      auto currMesh = snapshot.opaqueMeshHandles[i];
      auto currMat = snapshot.opaqueMaterials[i];

      if (currMesh == lastMesh && currMat == lastMaterial) {
        indirectCommands.GetBack().instanceCount++;
      } else {
        indirectCommands.PushBack({
            .indexCount = gpuData.indexCount,
            .instanceCount = 1,
            .firstIndex =
                gpuData.indexOffset / static_cast<uint32_t>(sizeof(uint)),
            .vertexOffset = 0,
            .firstInstance = static_cast<uint32_t>(instanceDatas.GetSize() - 1),
        });

        if (currMat != lastMaterial) {
          packet.opaqueBatches.PushBack({
              .material = {currMat.id},
              .commandOffset =
                  static_cast<uint32_t>(indirectCommands.GetSize() - 1),
              .commandCount = 0,
          });
          lastMaterial = currMat;
        }
        packet.opaqueBatches.GetBack().commandCount++;
        lastMesh = currMesh;
      }
    }

    auto instanceAlloc = ssboPool.AllocateAligned(instanceDatas.GetSize() *
                                                  sizeof(InstanceData));
    std::memcpy(instanceAlloc.pHostAddress, instanceDatas.GetData(),
                instanceAlloc.size);

    auto indirectAlloc = m_rhi.AllocateIndirectSSBO(
        indirectCommands.GetSize() * sizeof(IndexedIndirectCommand));
    std::memcpy(indirectAlloc.pHostAddress, indirectCommands.GetData(),
                indirectAlloc.size);

    packet.opaqueInstanceDataBaseOffset = instanceAlloc.offset;
    packet.indirectCommandBufferAllocation = indirectAlloc;
    packet.totalCommandCount = indirectCommands.GetSize();
  }

  graphics::InstanceData BuildInstanceData(uint32_t snapshotIdx,
                                           const SceneSnapshot &snapshot,
                                           uint32_t modelOffset,
                                           uint32_t invModelOffset) {
    graphics::InstanceData data{};

    auto meshHandle = snapshot.opaqueMeshHandles[snapshotIdx];
    auto instHandle = snapshot.opaqueMaterialInstances[snapshotIdx];
    auto *mesh = GetMeshManager().Resolve({meshHandle.id});
    auto &matInst = GetMaterialManager().ResolveSafe({instHandle.id});

    data.instanceID = snapshotIdx;
    data.materialID = matInst.GetGpuIndex();
    data.geometryOffset = mesh->GetPosUVOffset();
    data.attributeOffset = mesh->GetAttributeOffset();
    data.indexOffset = mesh->GetIndexOffset();
    data.vertexCount = mesh->GetVertexCount();
    data.indexCount = mesh->GetIndexCount();

    data.modelOffset = modelOffset;
    data.invModelOffset = invModelOffset;

    auto sdfType = mesh->GetSDFType();
    uint32_t index = 0;
    if (sdfType == ESDFType::Mesh) {
      auto handle = mesh->GetSDFTexture();
      if (handle.IsValid()) {
        index = m_rhi.GetBindlessManager().RegisterTexture3D(handle);
      }
    }

    data.sdfType = std::underlying_type_t<ESDFType>(sdfType);
    data.sdfTextureIndex = index;
    data.alphaThreshold = matInst.GetAlphaThreshold();

    data.sdfExtent = mesh->GetSDFExtent();

    return data;
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
