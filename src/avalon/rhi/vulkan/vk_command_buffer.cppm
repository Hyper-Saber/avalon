module;
#include <cstdint>
#include <debug/assert.hpp>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:command_buffer;

import avalon.rhi;
import avalon.core;
import :types;
import :utils;

namespace avalon::rhi {
class CommandBuffer final : public ICommandBuffer,
                            public mem::AutoDestroyable<CommandBuffer> {
public:
  CommandBuffer(VkCommandBuffer vkCmd, IRenderResourceProvider &provider,
                StateTracker &tracker)
      : m_cmd(vkCmd), m_resourceProvider(provider), m_stateTracker(tracker) {}

  ~CommandBuffer() override = default;

  VkCommandBuffer GetRaw() const { return m_cmd; }

  void Begin() override {
    m_lastBoundPipeline = {};
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    auto result = vkBeginCommandBuffer(m_cmd, &beginInfo);
    if (result != VK_SUCCESS) {
      Debug("[Vulkan]: Failed to begin command buffer!");
    }
  }

  void Dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ) override {
    FlushBarriers();
    vkCmdDispatch(m_cmd, groupCountX, groupCountY, groupCountZ);
  }

  void BindBindlessSet(EPipelineBindPoint point) override {
    if (m_layout == VK_NULL_HANDLE)
      return;

    VkDescriptorSet bindlessSet = m_resourceProvider.GetBindlessSet();
    if (bindlessSet != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(m_cmd, ToVkPipelineBindPoint(point), m_layout, 0,
                              1, &bindlessSet, 0, nullptr);
    }
  }

  void BeginRendering(const RenderingInfo &info) override {
    FlushBarriers();
    Array<VkRenderingAttachmentInfo> vkColors;
    for (const auto &col : info.colorAttachments) {
      auto *texRes = m_resourceProvider.GetTexture(col.texture);

      vkColors.PushBack({
          .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
          .pNext = nullptr,
          .imageView = texRes->imageView,
          .imageLayout = ToVkImageLayout(col.layout),
          .loadOp = ToVkLoadOp(col.loadOp),
          .storeOp = ToVkStoreOp(col.storeOp),
          .clearValue = {{{
              col.clearColor.r,
              col.clearColor.g,
              col.clearColor.b,
              col.clearColor.a,
          }}},
      });
    }

    VkRenderingAttachmentInfo vkDepth{};
    vkDepth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    bool hasDepth = info.depthStencil.has_value();
    bool hasStencil = false;

    if (hasDepth) {
      const auto &ds = info.depthStencil.value();
      auto *dsRes = m_resourceProvider.GetTexture(ds.texture);

      vkDepth.imageView = dsRes->imageView;
      vkDepth.imageLayout = ToVkImageLayout(ds.layout);
      vkDepth.loadOp = ToVkLoadOp(ds.loadOp);
      vkDepth.storeOp = ToVkStoreOp(ds.storeOp);
      vkDepth.clearValue = {.depthStencil = {ds.clearDepth, ds.clearStencil}};

      hasStencil = HasStencilComponent(dsRes->createInfo.format);
    }

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .renderArea = {.offset = {info.renderArea.offset.x,
                                  info.renderArea.offset.y},
                       .extent = {info.renderArea.extent.width,
                                  info.renderArea.extent.height}},
        .layerCount = info.layerCount,
        .viewMask = info.viewMask,
        .colorAttachmentCount = static_cast<uint32_t>(vkColors.GetSize()),
        .pColorAttachments = vkColors.GetData(),
        .pDepthAttachment = hasDepth ? &vkDepth : nullptr,
        .pStencilAttachment = hasStencil ? &vkDepth : nullptr};

    vkCmdBeginRendering(m_cmd, &renderingInfo);
  }

  void EndRendering() override {
    vkCmdEndRendering(m_cmd);
    FlushBarriers();
  }

  void SetViewport(const Viewport &viewport) override {
    VkViewport vkViewport{.x = viewport.x,
                          .y = viewport.y,
                          .width = viewport.width,
                          .height = viewport.height,
                          .minDepth = viewport.minDepth,
                          .maxDepth = viewport.maxDepth};

    vkCmdSetViewport(m_cmd, 0, 1, &vkViewport);
  }

  void SetScissor(const Rect2D &scissor) override {
    VkRect2D vkScissor{
        .offset = {.x = scissor.offset.x, .y = scissor.offset.y},
        .extent = {.width = scissor.extent.width,
                   .height = scissor.extent.height},
    };

    vkCmdSetScissor(m_cmd, 0, 1, &vkScissor);
  }

  void End() override { vkEndCommandBuffer(m_cmd); }

  void BindPipeline(PipelineHandle handle) override {
    if (m_lastBoundPipeline == handle)
      return;

    auto res = m_resourceProvider.GetPipeline(handle);
    m_layout = res->pipelineLayout;

    VkPipeline pipeline = res->pipeline;
    vkCmdBindPipeline(m_cmd, ToVkPipelineBindPoint(res->bindPoint), pipeline);
    m_lastBoundPipeline = handle;
  }

  void BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount,
                        const BufferHandle *pBuffers,
                        const uint64_t *pOffsets) override {
    VkDeviceSize defaultOffsets[16] = {0};
    const VkDeviceSize *pActualOffsets = pOffsets ? pOffsets : defaultOffsets;

    AVALON_ASSERT_MSG(bindingCount <= 16,
                      "Binding count is too high, max is 16");

    VkBuffer buffers[16];

    uint32_t actualCount = bindingCount > 16 ? 16 : bindingCount;

    for (uint32_t i = 0; i < actualCount; i++) {
      buffers[i] = m_resourceProvider.GetBuffer(pBuffers[i])->buffer;
    }

    vkCmdBindVertexBuffers(m_cmd, firstBinding, actualCount, buffers,
                           pActualOffsets);
  }

  void BindIndexBuffer(BufferHandle handle, uint64_t offset,
                       EFormat format) override {
    VkBuffer buffer = m_resourceProvider.GetBuffer(handle)->buffer;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    if (format == EFormat::R16_Uint) {
      indexType = VK_INDEX_TYPE_UINT16;
    }

    vkCmdBindIndexBuffer(m_cmd, buffer, offset, indexType);
  }

  void BindDescriptorSet(uint32_t firstSet,
                         Span<const DescriptorSetHandle> sets,
                         Span<const uint32_t> dynamicOffsets,
                         EPipelineBindPoint point) override {
    AVALON_ASSERT_MSG(firstSet > 0,
                      "[Vulkan]: Set 0 was binded by static set!");
    Array<VkDescriptorSet> vkSets;
    for (const auto &handle : sets) {
      auto *res = m_resourceProvider.GetDescriptorSet(handle);
      vkSets.PushBack(res->descriptorSet);
    }

    vkCmdBindDescriptorSets(m_cmd, ToVkPipelineBindPoint(point), m_layout,
                            firstSet, vkSets.GetSize(), vkSets.GetData(),
                            dynamicOffsets.GetSize(), dynamicOffsets.GetData());
  }

  void PushConstants(EShaderStage stage, uint32_t offset, uint32_t size,
                     const void *pData) override {
    VkShaderStageFlags vkStage = ToVkShaderStageFlags(stage);
    vkCmdPushConstants(m_cmd, m_layout, vkStage, offset, size, pData);
  }

  void UpdateBuffer(BufferHandle handle, uint64_t offset, const void *pData,
                    uint64_t size) override {
    VkBuffer buffer = m_resourceProvider.GetBuffer(handle)->buffer;
    vkCmdUpdateBuffer(m_cmd, buffer, offset, size, pData);
  }

  void FillBuffer(BufferHandle handle, uint32_t offset, uint32_t size,
                  uint32_t data) override {
    FlushBarriers();
    auto buffer = m_resourceProvider.GetBuffer(handle)->buffer;
    vkCmdFillBuffer(m_cmd, buffer, offset, size, data);
  }

  void CopyBuffer(BufferHandle src, BufferHandle dst,
                  const BufferCopyRegion &region) override {
    auto srcBuffer = m_resourceProvider.GetBuffer(src)->buffer;
    auto dstBuffer = m_resourceProvider.GetBuffer(dst)->buffer;

    VkBufferCopy vkRegin{
        .srcOffset = region.srcOffset,
        .dstOffset = region.dstOffset,
        .size = region.size,
    };

    vkCmdCopyBuffer(m_cmd, srcBuffer, dstBuffer, 1, &vkRegin);
  }

  void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance) override {
    FlushBarriers();
    vkCmdDraw(m_cmd, vertexCount, instanceCount, firstVertex, firstInstance);
  }

  void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                   uint32_t firstIndex, int32_t vertexOffset,
                   uint32_t firstInstance) override {
    FlushBarriers();
    vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, firstIndex, vertexOffset,
                     firstInstance);
  }

  void DrawIndirect(BufferHandle indirectBuffer, uint32_t offset,
                    uint32_t count, uint32_t stride) override {
    FlushBarriers();
    auto buffer = m_resourceProvider.GetBuffer(indirectBuffer)->buffer;
    vkCmdDrawIndirect(m_cmd, buffer, offset, count, stride);
  }

  void DrawIndexedIndirect(BufferHandle indirectBuffer, uint32_t offset,
                           uint32_t count, uint32_t stride) override {
    FlushBarriers();
    auto buffer = m_resourceProvider.GetBuffer(indirectBuffer)->buffer;
    vkCmdDrawIndexedIndirect(m_cmd, buffer, offset, count, stride);
  }

  void Transition(TextureHandle handle, EResourceUsage usage,
                  uint32_t layerCount, uint32_t levelCount,
                  EShaderStage stage) override {
    auto barrier = m_stateTracker.RequestSync(*this, handle, usage, layerCount,
                                              levelCount, stage);
    if (barrier.has_value()) {
      m_pendingImageBarriers.PushBack(barrier.value());
      m_isDirty = true;
    }
  }

  void SyncBuffer(BufferHandle handle, EResourceUsage usage, uint32_t offset,
                  uint32_t size, EShaderStage stage) override {
    auto barrier =
        m_stateTracker.RequestSync(*this, handle, usage, offset, size, stage);

    if (barrier.has_value()) {
      m_pendingBufferBarriers.PushBack(barrier.value());
      m_isDirty = true;
    }
  }

  void PipelineBarrier(Span<const ImageBarrier> barriers) override {
    Array<VkImageMemoryBarrier2> vkBarriers;
    TextureHandle lastHandle;
    VkImageAspectFlags aspectMask;
    VkImage image;
    for (auto &barrier : barriers) {
      if (barrier.texture != lastHandle) {
        lastHandle = barrier.texture;
        auto pRes = m_resourceProvider.GetTexture(barrier.texture);
        image = pRes->image;
        aspectMask = pRes->aspectMask;
      }

      VkImageMemoryBarrier2 vkBarrier{
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .pNext = nullptr,
          .srcStageMask = ToVkPipelineStageFlags(barrier.srcStage),
          .srcAccessMask = ToVkAccessFlags(barrier.srcAccess),
          .dstStageMask = ToVkPipelineStageFlags(barrier.dstStage),
          .dstAccessMask = ToVkAccessFlags(barrier.dstAccess),
          .oldLayout = ToVkImageLayout(barrier.oldLayout),
          .newLayout = ToVkImageLayout(barrier.newLayout),
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = image,
          .subresourceRange =
              {
                  .aspectMask = aspectMask,
                  .baseMipLevel = barrier.baseMipLevel,
                  .levelCount = barrier.levelCount,
                  .baseArrayLayer = barrier.baseArrayLayer,
                  .layerCount = barrier.layerCount,
              },
      };
      vkBarriers.PushBack(vkBarrier);
    }

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = static_cast<uint32_t>(vkBarriers.GetSize()),
        .pImageMemoryBarriers = vkBarriers.GetData(),
    };

    vkCmdPipelineBarrier2(m_cmd, &depInfo);
  }

  void CopyImage(TextureHandle src, TextureHandle dst,
                 const ImageCopyRegion &region) override {
    FlushBarriers();
    auto srcTextureRes = m_resourceProvider.GetTexture({src.id});
    auto dstTextureRes = m_resourceProvider.GetTexture({dst.id});

    VkImageCopy copyRegion{
        .srcSubresource =
            {
                .aspectMask = srcTextureRes->aspectMask,
                .mipLevel = region.srcMipLevel,
                .baseArrayLayer = region.srcLayer,
                .layerCount = region.layerCount,
            },
        .srcOffset = {region.srcOffset.x, region.srcOffset.y, 0},
        .dstSubresource =
            {
                .aspectMask = dstTextureRes->aspectMask,
                .mipLevel = region.dstMipLevel,
                .baseArrayLayer = region.dstLayer,
                .layerCount = region.layerCount,
            },
        .dstOffset = {region.dstOffset.x, region.dstOffset.y, 0},
        .extent = {region.extent.width, region.extent.height, 1}};

    vkCmdCopyImage(m_cmd, srcTextureRes->image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureRes->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
  }

  void Blit(TextureHandle src, TextureHandle dst,
            ImageBlitRegion region) override {
    auto pSrcRes = m_resourceProvider.GetTexture({src.id});
    auto pDstRes = m_resourceProvider.GetTexture(dst);
    VkImageBlit2 vkRegion{
        .srcSubresource{
            .aspectMask = pSrcRes->aspectMask,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 6,
        },
        .dstSubresource{
            .aspectMask = pDstRes->aspectMask,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 6,
        },
    };

    VkBlitImageInfo2 info{
        .srcImage = pSrcRes->image,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = pDstRes->image,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .pRegions = &vkRegion,
        .filter = VK_FILTER_CUBIC_IMG,
    };
    vkCmdBlitImage2(m_cmd, &info);
  }

  void ClearColor(TextureHandle handle, ClearValue value) override {
    FlushBarriers();
    auto res = m_resourceProvider.GetTexture({handle.id});
    auto color = value.color;
    VkClearColorValue clearColor = {{color.r, color.g, color.b, color.a}};
    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = res->createInfo.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = res->createInfo.layerCount,
    };
    vkCmdClearColorImage(m_cmd, res->image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1,
                         &range);
  }

private:
  void FlushBarriers() {
    if (!m_isDirty)
      return;
    InternalPipelineBarrier(m_pendingImageBarriers, m_pendingBufferBarriers);
    m_pendingImageBarriers.Clear();
    m_pendingBufferBarriers.Clear();
    m_isDirty = false;
  }

  void InternalPipelineBarrier(Span<const ImageBarrier> imageBarriers,
                               Span<const BufferBarrier> bufferBarriers) {
    if (imageBarriers.IsEmpty() && bufferBarriers.IsEmpty())
      return;

    Array<VkImageMemoryBarrier2> vkBarriers;

    for (const auto &b : imageBarriers) {
      auto textureRes = m_resourceProvider.GetTexture(b.texture);

      VkImageMemoryBarrier2 vkBarrier{
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .pNext = nullptr,
          .srcStageMask = ToVkPipelineStageFlags(b.srcStage),
          .srcAccessMask = ToVkAccessFlags(b.srcAccess),
          .dstStageMask = ToVkPipelineStageFlags(b.dstStage),
          .dstAccessMask = ToVkAccessFlags(b.dstAccess),
          .oldLayout = ToVkImageLayout(b.oldLayout),
          .newLayout = ToVkImageLayout(b.newLayout),
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = textureRes->image,
          .subresourceRange = {
              .aspectMask = textureRes->aspectMask,
              .baseMipLevel = b.baseMipLevel,
              .levelCount = b.levelCount,
              .baseArrayLayer = b.baseArrayLayer,
              .layerCount = b.layerCount,
          }};
      vkBarriers.PushBack(vkBarrier);
      // Debug("Transition layout texture:{}, src: {}, dest: {}, baseMipLevel: "
      //       "{}, levelCount: {}",
      //       textureRes->createInfo.nameHash.Resolve(), ToView(b.oldLayout),
      //       ToView(b.newLayout), b.baseMipLevel, b.levelCount);
    }

    Array<VkBufferMemoryBarrier2> vkBufferBarriers;
    for (auto &b : bufferBarriers) {
      auto pBufferRes = m_resourceProvider.GetBuffer(b.buffer);
      VkBufferMemoryBarrier2 vkBufferBarrier{
          .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
          .srcStageMask = ToVkPipelineStageFlags(b.srcStage),
          .srcAccessMask = ToVkAccessFlags(b.srcAccess),
          .dstStageMask = ToVkPipelineStageFlags(b.dstStage),
          .dstAccessMask = ToVkAccessFlags(b.dstAccess),
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .buffer = pBufferRes->buffer,
          .offset = b.offset,
          .size = b.size,
      };
      vkBufferBarriers.PushBack(vkBufferBarrier);
      // Debug("buffer barrier: handle: {}, offset: {}, size: {}, srcStage: {},
      // "
      //       "dstStage: {}, srcAccess: {}, dstAccess{}",
      //       b.buffer.id, b.offset, b.size, ToView(b.srcStage),
      //       ToView(b.dstStage), ToView(b.srcAccess), ToView(b.dstAccess));
    }

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount =
            static_cast<uint32_t>(vkBufferBarriers.GetSize()),
        .pBufferMemoryBarriers = vkBufferBarriers.GetData(),
        .imageMemoryBarrierCount = static_cast<uint32_t>(vkBarriers.GetSize()),
        .pImageMemoryBarriers = vkBarriers.GetData(),
    };

    vkCmdPipelineBarrier2(m_cmd, &depInfo);
  }

  struct ResourceFinalStateSnapshot {
    TextureHandle handle;
    EResourceUsage usage;
    EResourceLayout layout;
  };

  bool m_isDirty;
  const VkCommandBuffer m_cmd;
  IRenderResourceProvider &m_resourceProvider;
  PipelineHandle m_lastBoundPipeline{};
  VkPipelineLayout m_layout{VK_NULL_HANDLE};

  Array<ImageBarrier> m_pendingImageBarriers;
  Array<BufferBarrier> m_pendingBufferBarriers;

  StateTracker &m_stateTracker;
};

} // namespace avalon::rhi
