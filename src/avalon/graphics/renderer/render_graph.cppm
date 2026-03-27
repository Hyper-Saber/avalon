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
  }

  auto ImportExternalTexture(StringId name, rhi::TextureHandle physicalHandle,
                             VirtualTextureDesc desc) -> VirtualResourceHandle {
    auto index =
        m_resourceManager.ImportExternalTexture(name, physicalHandle, desc);
    auto handle = m_resourceManager.GetVirtualResource(index);
    m_resourceManager.RefineUsage(handle, desc.usage);

    return handle;
  }

  void Compile() {
    HashMap<uint32_t, uint32_t> producerMap;

    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      for (auto request : m_nodes[i].outputs) {
        producerMap.Insert(request.handle.GetIndex(), i);
      }
    }

    for (auto &node : m_nodes)
      node.isCulled = true;

    auto MarkNodeActive = [&](this auto &&self, uint32_t nodeIndex) -> void {
      auto &node = m_nodes[nodeIndex];
      if (!node.isCulled)
        return;

      node.isCulled = false;
      for (auto request : node.inputs) {
        if (producerMap.Contains(request.handle.GetIndex())) {
          self(*producerMap.Get(request.handle.GetIndex()));
        }
      }
    };

    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      bool hasSideEffect =
          std::ranges::any_of(m_nodes[i].outputs, [&](auto request) {
            return request.handle.IsExternal();
          });
      if (hasSideEffect)
        MarkNodeActive(i);
    }

    // kahn sort
    Array<uint32_t> inDegrees(m_nodes.GetSize());
    Array<Array<uint32_t>> adjacencyList(m_nodes.GetSize());
    for (uint32_t i = 0; i < m_nodes.GetSize(); i++) {
      if (m_nodes[i].isCulled)
        continue;
      for (auto request : m_nodes[i].inputs) {
        if (auto *pProducerIndex = producerMap.Get(request.handle.GetIndex())) {
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
    m_resourceManager.RealizeResources(timelines);

    for (auto &nodeIndex : m_executionQueue) {
      auto &node = m_nodes[nodeIndex];
      node.pass->OnCompile(m_rhi);
    }
  }

  void Render(rhi::ICommandBuffer &cmd, RenderContext &context) {
    HashMap<rhi::EResourceUsage, rhi::TextureHandle> finalPendingUsages;

    bool isGlobalSetBinded = false;

    for (uint32_t nodeIdx : m_executionQueue) {
      auto &node = m_nodes[nodeIdx];
      context.pipelineRenderingInfo.Clear();
      rhi::RenderingInfo renderingInfo{.renderArea = node.renderArea,
                                       .layerCount = 1};

      for (auto &output : node.outputs) {
        auto physicalHandle = m_resourceManager.GetPhysical(output.handle);
        if (!physicalHandle.IsValid())
          continue;

        auto fixedUsage = output.usage;
        if (fixedUsage == rhi::EResourceUsage::Present) {
          fixedUsage = rhi::EResourceUsage::ColorAttachment;
          finalPendingUsages.Insert(output.usage, physicalHandle);
        }
        cmd.Transition(physicalHandle, fixedUsage);

        auto resDesc = m_resourceManager.GetResourceDesc(output.handle);

        if (IsDepthFormat(resDesc.format)) {
          renderingInfo.depthStencil = rhi::DepthStencilAttachmentInfo{
              .texture = physicalHandle,
              .loadOp = output.loadOp,
              .storeOp = output.storeOp,
              .clearDepth = output.clearValue.depthStencil.depth,
              .clearStencil = output.clearValue.depthStencil.stencil};
          context.pipelineRenderingInfo.depthAttachmentFormat = resDesc.format;
          if (HasStencilComponent(resDesc.format))
            context.pipelineRenderingInfo.stencilAttachmentFormat =
                resDesc.format;
        } else {
          renderingInfo.colorAttachments.PushBack(
              {.texture = physicalHandle,
               .loadOp = output.loadOp,
               .storeOp = output.storeOp,
               .clearColor = output.clearValue.color});
          context.pipelineRenderingInfo.colorAttachmentFormats.PushBack(
              resDesc.format);
        }
      }

      for (auto &input : node.inputs) {
        auto physicalHandle = m_resourceManager.GetPhysical(input.handle);
        if (physicalHandle.IsValid()) {
          cmd.Transition(physicalHandle, input.usage);
        }
      }

      cmd.BeginRendering(renderingInfo);
      if (!isGlobalSetBinded) {
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
          .minDepth = 1.0f,
          .maxDepth = 0.0f,
      };

      cmd.SetViewport(port);
      cmd.SetScissor(node.renderArea);
      node.pass->Execute(cmd, context);
      for (auto &entry : finalPendingUsages) {
        auto usage = entry.GetKey();
        auto handle = entry.GetValue();
        cmd.Transition(handle, usage);
      }
      cmd.EndRendering();
    }
  }

  friend class RenderGraphBuilder;

private:
  struct ResourceRequest {
    StringId nameHash;
    VirtualResourceHandle handle;
    rhi::EResourceUsage usage;
    rhi::EAttachmentLoadOp loadOp = rhi::EAttachmentLoadOp::DontCare;
    rhi::EAttachmentStoreOp storeOp = rhi::EAttachmentStoreOp::DontCare;
    rhi::ClearValue clearValue;
  };

  struct PassNode {
    StringId nameHash;
    UniquePtr<IRenderPass> pass;
    Array<ResourceRequest> inputs;
    Array<ResourceRequest> outputs;

    Rect2D renderArea;
    bool isCulled = false;
  };

  PassNode &GetNode(IRenderPass *pass) {
    if (m_nodeIndices.Contains(pass)) {
      return m_nodes[*m_nodeIndices.Get(pass)];
    }
    return m_nodes[0];
  }

  template <TRenderPass T, typename... Args>
  T &AddPass(StringId name, Args &&...args) {
    auto pass = MakeUnique<T>(std::forward<Args>(args)...);
    T &passRef = *pass;
    m_nodes.PushBack(PassNode{.nameHash = name, .pass = std::move(pass)});
    m_nodeIndices.Insert(&passRef, m_nodes.GetSize() - 1);
    return passRef;
  }

  auto Write(IRenderPass &owner, VirtualTextureDesc desc)
      -> VirtualResourceHandle {
    auto handleIndex = m_resourceManager.GetOrCreateVirtualResouceIndex(desc);
    auto &node = GetNode(&owner);
    auto handle = m_resourceManager.IncreaseGeneration(handleIndex);
    node.outputs.PushBack({.handle = handle, .usage = desc.usage});
    node.renderArea.extent = desc.extent;
    m_resourceManager.RefineUsage(handle, desc.usage);
    return handle;
  }

  auto Read(IRenderPass &owner, StringId id, rhi::EResourceUsage usage)
      -> VirtualResourceHandle {
    auto handle = m_resourceManager.GetVirtualResource(id);
    auto &node = GetNode(&owner);
    node.inputs.PushBack({.handle = handle, .usage = usage});
    m_resourceManager.RefineUsage(handle, usage);
    return handle;
  }

  rhi::IRhi &m_rhi;
  VirtualResourceManager &m_resourceManager;
  Array<PassNode> m_nodes;
  Array<uint32_t> m_executionQueue;
  HashMap<IRenderPass *, uint32_t> m_nodeIndices;

  HashMap<StringId, VirtualTextureDesc> m_textureDescs;

  rhi::PipelineHandle m_dummyPipline;
};
} // namespace avalon::graphics
