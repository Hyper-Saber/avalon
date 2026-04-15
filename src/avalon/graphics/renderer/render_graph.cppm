module;
#include <algorithm>
#include <cstdint>
#include <utility>

export module avalon.graphics:render_graph;

import avalon.core;
import avalon.rhi;
import :renderer_types;
import :render_pass;
import :virtual_resource_manager;
import :utils;

namespace avalon::graphics {

class RenderGraph final : public NonCopyable {
public:
  explicit RenderGraph(rhi::IRhi &rhi, VirtualResourceManager &manager)
      : m_rhi(rhi), m_resourceManager(manager) {
    auto &mm = GetMaterialManager();
    m_dummyPipline =
        mm.Resolve(mm.GetDefaultOpaque())->GetOrCreatePipeline(rhi, {});
    m_dummyComputePipeline = rhi.GetDummyComputePipeline();
  }

  auto ImportExternalTexture(StringId name, rhi::TextureHandle physicalHandle,
                             VirtualTextureDesc desc) -> VirtualResourceHandle {
    auto index =
        m_resourceManager.ImportExternalTexture(name, physicalHandle, desc);
    auto handle = m_resourceManager.GetVirtualTexture(index);

    return handle;
  }

  void Compile() {
    HashMap<VirtualResourceHandle, uint32_t> producerMap;
    HashMap<VirtualResourceHandle, uint32_t> bufferProducerMap;

    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      for (auto &output : m_nodes[i].outputs) {
        producerMap.Insert(output.handle, i);
      }
      for (auto &output : m_nodes[i].bufferOutputs) {
        bufferProducerMap.Insert(output.handle, i);
      }
    }

    for (auto &node : m_nodes)
      node.isCulled = true;

    auto MarkNodeActive = [&](this auto &&self, uint32_t nodeIndex) -> void {
      auto &node = m_nodes[nodeIndex];
      if (!node.isCulled)
        return;

      node.isCulled = false;
      // Debug("Mark node {} active", node.nameHash.Resolve());
      for (auto &request : node.inputs) {
        if (producerMap.Contains(request.handle)) {
          self(*producerMap.Get(request.handle));
        }
      }
      for (auto &input : node.bufferInputs) {
        if (bufferProducerMap.Contains(input.handle)) {
          self(*bufferProducerMap.Get(input.handle));
        }
      }
    };

    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      bool hasSideEffect =
          std::ranges::any_of(
              m_nodes[i].outputs,
              [&](auto request) { return request.handle.IsExternal(); }) ||
          std::ranges::any_of(m_nodes[i].bufferOutputs, [&](auto request) {
            return request.handle.IsExternal();
          });
      if (hasSideEffect) {
        // Debug("Side node: {}", m_nodes[i].nameHash.Resolve());
        MarkNodeActive(i);
      }
    }

    // kahn sort
    Array<uint32_t> inDegrees(m_nodes.GetSize());
    Array<Array<uint32_t>> adjacencyList(m_nodes.GetSize());
    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      if (m_nodes[i].isCulled)
        continue;
      for (auto request : m_nodes[i].inputs) {
        if (auto *pProducerIndex = producerMap.Get(request.handle)) {
          adjacencyList[*pProducerIndex].PushBack(i);
          inDegrees[i]++;
        }
      }
      for (auto request : m_nodes[i].bufferInputs) {
        if (auto *pProducerIndex = bufferProducerMap.Get(request.handle)) {
          adjacencyList[*pProducerIndex].PushBack(i);
          inDegrees[i]++;
        }
      }
    }

    Array<uint32_t> readyNodes;
    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      if (!m_nodes[i].isCulled && inDegrees[i] == 0) {
        readyNodes.PushBack(i);
      }
    }

    m_executionQueue.Clear();
    while (!readyNodes.IsEmpty()) {
      uint32_t currIndex = readyNodes.GetBack();
      readyNodes.PopBack();
      m_executionQueue.PushBack(currIndex);

      for (uint32_t neighborIndex : adjacencyList[currIndex]) {
        inDegrees[neighborIndex]--;
        if (inDegrees[neighborIndex] == 0) {
          readyNodes.PushBack(neighborIndex);
        }
      }
    }

    HashMap<VirtualResourceHandle, ResourceTimeline> timelines;
    for (uint32_t order = 0; order < m_executionQueue.GetSize(); order++) {
      uint32_t nodeIndex = m_executionQueue[order];

      auto &node = m_nodes[nodeIndex];

      auto RegisterUsage = [&](VirtualResourceHandle handle) {
        if (!timelines.Contains(handle))
          timelines.Insert(handle,
                           {.firstPassIndex = order, .lastPassIndex = order});
        else
          timelines.Get(handle)->lastPassIndex = order;
      };

      for (auto request : node.inputs)
        RegisterUsage(request.handle);
      for (auto request : node.outputs)
        RegisterUsage(request.handle);
    }
    m_resourceManager.RealizeTextures(timelines);

    for (auto &nodeIndex : m_executionQueue) {
      auto &node = m_nodes[nodeIndex];
      node.pass->OnCompile(m_rhi);
    }
  }

  void Render(rhi::ICommandBuffer &cmd, RenderContext &context) {
    // Debug("-------------------------------render graph "
    //       "begin------------------------------------");

    Array<PendingUsage> finalPendingUsages;

    bool isGlobalSetBinded = false;
    bool isComputeGlobalSetBinded = false;

    for (uint32_t nodeIdx : m_executionQueue) {
      auto &node = m_nodes[nodeIdx];

      // if (node.nameHash == "Shadow"_id) {
      //   Debug("{}", node.nameHash.Resolve());
      // }

      HandleResourceTransitions(cmd, node, finalPendingUsages);

      if (node.passType == rhi::EPassType::Compute) {
        if (!isComputeGlobalSetBinded) [[unlikely]] {
          cmd.BindPipeline(m_dummyComputePipeline);
          cmd.BindBindlessSet(rhi::EPipelineBindPoint::Compute);
          cmd.BindDescriptorSet(kSceneGlobalsSet, {&context.sceneGlobalsSet, 1},
                                {&context.sceneGlobalsSetDynamicOffset, 1},
                                rhi::EPipelineBindPoint::Compute);
          isComputeGlobalSetBinded = true;
        }
        node.pass->Execute(cmd, context);
        continue;
      }

      context.pipelineRenderingInfo.Clear();
      context.pipelineRenderingInfo.viewMask = node.viewMask;
      rhi::RenderingInfo renderingInfo{
          .renderArea = node.renderArea,
          .layerCount = node.layerCount,
          .viewMask = node.viewMask,
      };

      for (auto &output : node.outputs) {
        auto physicalHandle =
            m_resourceManager.GetPhysicalTexture(output.handle);
        if (!physicalHandle.IsValid())
          continue;

        auto &resDesc = m_resourceManager.GetTextureDesc(output.handle);
        auto fixedUsage = FixUsage(output.initialUsage);

        if (IsDepthFormat(resDesc.format)) {
          renderingInfo.depthStencil = rhi::DepthStencilAttachmentInfo{
              .texture = physicalHandle,
              .loadOp = output.loadOp,
              .storeOp = output.storeOp,
              .clearDepth = output.clearValue.depthStencil.depth,
              .clearStencil = output.clearValue.depthStencil.stencil,
              .layout = MapUsageToLayout(fixedUsage),
          };
          context.pipelineRenderingInfo.depthAttachmentFormat = resDesc.format;
          if (HasStencilComponent(resDesc.format))
            context.pipelineRenderingInfo.stencilAttachmentFormat =
                resDesc.format;
        } else {
          renderingInfo.colorAttachments.PushBack({
              .texture = physicalHandle,
              .loadOp = output.loadOp,
              .storeOp = output.storeOp,
              .clearColor = output.clearValue.color,
              .layout = MapUsageToLayout(fixedUsage),
          });
          context.pipelineRenderingInfo.colorAttachmentFormats.PushBack(
              resDesc.format);
        }
      }
      cmd.BeginRendering(renderingInfo);
      if (!isGlobalSetBinded) [[unlikely]] {
        cmd.BindPipeline(m_dummyPipline);
        cmd.BindBindlessSet();
        cmd.BindDescriptorSet(kSceneGlobalsSet, {&context.sceneGlobalsSet, 1},
                              {&context.sceneGlobalsSetDynamicOffset, 1});
        isGlobalSetBinded = true;
      }
      Viewport port{
          .x = static_cast<float>(node.renderArea.offset.x),
          .y = static_cast<float>(node.renderArea.offset.y),
          .width = static_cast<float>(node.renderArea.extent.width),
          .height = static_cast<float>(node.renderArea.extent.height),
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
      };

      cmd.SetViewport(port);
      cmd.SetScissor(node.renderArea);

      node.pass->Execute(cmd, context);

      for (auto &entry : finalPendingUsages) {
        cmd.Transition(entry.handle, entry.usage, entry.layerCount,
                       entry.levelCount, entry.stage);
      }

      cmd.EndRendering();
    }
    // Debug("-------------------------------render graph "
    //       "end------------------------------------");
  }

  friend class RenderGraphBuilder;

private:
  struct textureRequest {
    StringId nameHash;
    VirtualResourceHandle handle;
    rhi::EResourceUsage initialUsage = rhi::EResourceUsage::None;
    EShaderStage stage;
    rhi::EAttachmentLoadOp loadOp = rhi::EAttachmentLoadOp::DontCare;
    rhi::EAttachmentStoreOp storeOp = rhi::EAttachmentStoreOp::DontCare;
    rhi::ClearValue clearValue;
  };

  struct BufferRequest {
    StringId nameHash;
    VirtualResourceHandle handle;
    rhi::EResourceUsage initialUsage = rhi::EResourceUsage::None;
    EShaderStage stage;
  };

  struct PassNode {
    StringId nameHash;
    UniquePtr<IRenderPass> pass;
    Array<textureRequest> inputs;
    Array<textureRequest> outputs;
    Array<BufferRequest> bufferInputs;
    Array<BufferRequest> bufferOutputs;

    Rect2D renderArea;
    uint32_t layerCount = 1;
    uint32_t levelCount = 1;
    uint32_t viewMask = 0;
    bool isCulled = false;

    rhi::EPassType passType = rhi::EPassType::Graphics;
  };

  struct PendingUsage {
    rhi::EResourceUsage usage;
    rhi::TextureHandle handle;
    uint32_t layerCount;
    uint32_t levelCount;
    EShaderStage stage;
  };

  PassNode &GetNode(IRenderPass *pass) {
    if (m_nodeIndices.Contains(pass)) {
      return m_nodes[*m_nodeIndices.Get(pass)];
    }
    return m_nodes[0];
  }

  template <TRenderPass T, typename... Args>
  T &AddPass(StringId name, rhi::EPassType type, Args &&...args) {
    auto pass = MakeUnique<T>(std::forward<Args>(args)...);
    T &passRef = *pass;
    m_nodes.PushBack(
        PassNode{.nameHash = name, .pass = std::move(pass), .passType = type});
    m_nodeIndices.Insert(&passRef, m_nodes.GetSize() - 1);
    return passRef;
  }

  auto Write(IRenderPass &owner, VirtualTextureDesc desc,
             rhi::EResourceUsage initialUsage, EShaderStage stage)
      -> VirtualResourceHandle {

    auto handleIndex =
        m_resourceManager.GetOrCreateVirtualTextureResouceIndex(desc);
    auto &node = GetNode(&owner);
    if (!m_resourceManager.IsFirstTextureGeneration(handleIndex)) {
      auto handle = m_resourceManager.GetVirtualTexture(handleIndex);
      if (!handle.IsExternal()) {
        node.inputs.PushBack({
            .nameHash = desc.nameHash,
            .handle = m_resourceManager.GetVirtualTexture(handleIndex),
        });
      }
    }
    auto handle = m_resourceManager.IncreaseTextureGeneration(handleIndex);
    node.outputs.PushBack({
        .nameHash = desc.nameHash,
        .handle = handle,
        .initialUsage = initialUsage,
        .stage = stage,
    });

    node.renderArea.extent = desc.extent;
    m_resourceManager.RefineTextureUsage(handle, desc.usage);
    return handle;
  }

  auto Read(IRenderPass &owner, StringId id, rhi::EResourceUsage usage,
            rhi::EResourceUsage initialUsage, EShaderStage stage)
      -> VirtualResourceHandle {
    auto handle = m_resourceManager.GetVirtualResource(id);
    auto &node = GetNode(&owner);
    node.inputs.PushBack({
        .nameHash = id,
        .handle = handle,
        .initialUsage = initialUsage,
        .stage = stage,
    });
    m_resourceManager.RefineTextureUsage(handle, usage);
    return handle;
  }

  auto WriteBuffer(IRenderPass &owner, StringId nameHash,
                   rhi::EResourceUsage usage, rhi::EResourceUsage initialUsage,
                   uint32_t size, EShaderStage stage) -> VirtualResourceHandle {
    auto index =
        m_resourceManager.GetOrCreateVirtualBufferIndex(nameHash, usage, size);
    auto &node = GetNode(&owner);
    if (!m_resourceManager.IsFirstBufferGeneration(index)) {
      auto handle = m_resourceManager.GetVirtualBuffer(index);
      if (!handle.IsExternal()) {
        node.bufferInputs.PushBack({
            .nameHash = nameHash,
            .handle = handle,
        });
      }
    }
    auto handle = m_resourceManager.IncreaseBufferGeneration(index);
    node.bufferOutputs.PushBack({
        .nameHash = nameHash,
        .handle = handle,
        .initialUsage = initialUsage,
        .stage = stage,
    });
    return handle;
  }

  auto ReadBuffer(IRenderPass &owner, StringId nameHash,
                  rhi::EResourceUsage initialUsage, EShaderStage stage) {
    auto handle = m_resourceManager.GetVirtualBuffer(nameHash);
    auto &node = GetNode(&owner);
    node.bufferInputs.PushBack({
        .nameHash = nameHash,
        .handle = handle,
        .initialUsage = initialUsage,
        .stage = stage,
    });
    return handle;
  }

  void HandleResourceTransitions(ICommandBuffer &cmd, PassNode &node,
                                 Array<PendingUsage> &finalPendingUsages) {
    for (auto &output : node.outputs) {
      auto physicalHandle = m_resourceManager.GetPhysicalTexture(output.handle);
      if (!physicalHandle.IsValid())
        continue;

      auto &desc = m_resourceManager.GetTextureDesc(output.handle);
      auto fixedUsage = FixUsage(output.initialUsage);
      if (fixedUsage != output.initialUsage) {
        finalPendingUsages.PushBack({output.initialUsage, physicalHandle,
                                     desc.layerCount, desc.mipLevels,
                                     output.stage});
      }

      // Debug(
      //     "Transitioning resource {}({}): to {}. layerCount: {}, mipLevels:
      //     {}", output.nameHash.Resolve(),
      //     m_resourceManager.GetPhysical(output.handle).id,
      //     ToView(fixedUsage), desc.layerCount, desc.mipLevels);

      cmd.Transition(physicalHandle, fixedUsage, desc.layerCount,
                     desc.mipLevels, output.stage);
    }

    for (auto &input : node.inputs) {
      if (input.initialUsage == rhi::EResourceUsage::None)
        continue;
      auto physicalHandle = m_resourceManager.GetPhysicalTexture(input.handle);
      if (physicalHandle.IsValid()) {
        auto desc = m_resourceManager.GetTextureDesc(input.handle);
        // Debug("Transitioning resource {}({}): to {}. layerCount: {}, "
        //       "mipLevels: {}",
        //       input.nameHash.Resolve(),
        //       m_resourceManager.GetPhysical(input.handle).id,
        //       ToView(input.initialUsage), desc.layerCount, desc.mipLevels);
        cmd.Transition(physicalHandle, input.initialUsage, desc.layerCount,
                       desc.mipLevels, input.stage);
      }
    }

    for (auto &output : node.bufferOutputs) {
      auto allocation =
          m_resourceManager.GetPhysicalBufferAllocation(output.handle);
      if (allocation.has_value()) {
        cmd.SyncBuffer(allocation->buffer, output.initialUsage,
                       allocation->offset, allocation->size, output.stage);
      }
    }

    for (auto &input : node.bufferInputs) {
      auto allocation =
          m_resourceManager.GetPhysicalBufferAllocation(input.handle);
      if (allocation.has_value()) {
        cmd.SyncBuffer(allocation->buffer, input.initialUsage,
                       allocation->offset, allocation->size, input.stage);
      }
    }
  }

  auto FixUsage(rhi::EResourceUsage usage) -> rhi::EResourceUsage {
    auto fixedUsage = usage;
    if (fixedUsage == rhi::EResourceUsage::Present) {
      fixedUsage = rhi::EResourceUsage::ColorAttachment;
    }
    return fixedUsage;
  }

  rhi::IRhi &m_rhi;
  VirtualResourceManager &m_resourceManager;
  Array<PassNode> m_nodes;
  Array<uint32_t> m_executionQueue;
  HashMap<IRenderPass *, uint32_t> m_nodeIndices;

  HashMap<StringId, VirtualTextureDesc> m_textureDescs;

  rhi::PipelineHandle m_dummyPipline;
  rhi::PipelineHandle m_dummyComputePipeline;
};
} // namespace avalon::graphics
