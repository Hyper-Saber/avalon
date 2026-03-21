module;
#include <array>
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
  CommandBuffer(VkCommandBuffer vkCmd, IRenderResourceProvider &provider)
      : m_cmd(vkCmd), m_resourceProvider(provider) {}

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

  void BeginRenderPass(const RenderPassBeginInfo &info) override {
    Array<VkClearValue> clearValues(info.clearValues.GetSize());
    for (uint32_t i = 0; i < info.clearValues.GetSize(); i++) {
      if (info.clearValues[i].isDepth) {
        clearValues[i].depthStencil = {
            .depth = info.clearValues[i].depthStencil.depth,
            .stencil = info.clearValues[i].depthStencil.stencil,
        };
      } else {
        clearValues[i].color = {
            {info.clearValues[i].color.r, info.clearValues[i].color.g,
             info.clearValues[i].color.b, info.clearValues[i].color.a}};
      }
    }

    auto renderPassRes =
        m_resourceProvider.GetRenderPass(info.renderPassHandle);

    VkRenderPassBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPassRes->renderPass,
        .framebuffer = m_resourceProvider
                           .GetFrameBuffer(info.renderPassHandle,
                                           *renderPassRes, info.targets)
                           ->frameBuffer,
        .renderArea = {.offset = {info.renderArea.offset.x,
                                  info.renderArea.offset.y},
                       .extent = {info.renderArea.extent.width,
                                  info.renderArea.extent.height}},
        .clearValueCount = static_cast<uint32_t>(clearValues.GetSize()),
        .pClearValues = clearValues.GetData()};

    vkCmdBeginRenderPass(m_cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
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

  void EndRenderPass() override { vkCmdEndRenderPass(m_cmd); }

  void End() override { vkEndCommandBuffer(m_cmd); }

  void BindPipeline(PipelineHandle handle) override {
    if (m_lastBoundPipeline == handle)
      return;

    auto res = m_resourceProvider.GetPipeline(handle);
    VkPipeline pipeline = res->pipeline;
    vkCmdBindPipeline(
        m_cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    m_lastBoundPipeline = handle;
    m_layout = res->pipelineLayout;
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
                         Span<const uint32_t> dynamicOffsets) override {
    if (sets.IsEmpty())
      return;

    Array<VkDescriptorSet> vkSets;

    for (const auto &handle : sets) {
      auto setResouce = m_resourceProvider.GetDescriptorSet(handle);
      vkSets.PushBack(setResouce->descriptorSet);
    }

    vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout,
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

  void CopyBuffer(BufferHandle src, BufferHandle dst,
                  const BufferCopy &region) override {
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
    vkCmdDraw(m_cmd, vertexCount, instanceCount, firstVertex, firstInstance);
  }

  void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                   uint32_t firstIndex, int32_t vertexOffset,
                   uint32_t firstInstance) override {
    vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, firstIndex, vertexOffset,
                     firstInstance);
  }

private:
  const VkCommandBuffer m_cmd;
  IRenderResourceProvider &m_resourceProvider;
  PipelineHandle m_lastBoundPipeline;
  VkPipelineLayout m_layout;
};

} // namespace avalon::rhi
