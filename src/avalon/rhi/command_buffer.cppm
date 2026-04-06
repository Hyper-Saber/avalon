module;
#include <cstdint>
export module avalon.rhi:command_buffer;
import avalon.core;
import :types;

export namespace avalon::rhi {
class ICommandBuffer {
public:
  virtual ~ICommandBuffer() = default;
  virtual void Begin() = 0;
  virtual void
      BindBindlessSet(EPipelineBindPoint = EPipelineBindPoint::graphics) = 0;
  virtual void SetViewport(const Viewport &viewport) = 0;
  virtual void SetScissor(const Rect2D &scissor) = 0;
  virtual void BeginRendering(const RenderingInfo &info) = 0;
  virtual void EndRendering() = 0;
  virtual void End() = 0;

  virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY,
                        uint32_t groupCountZ) = 0;

  virtual void
  BindPipeline(PipelineHandle handle,
               EPipelineBindPoint point = EPipelineBindPoint::graphics) = 0;
  virtual void BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount,
                                const BufferHandle *pBuffers,
                                const uint64_t *pOffsets) = 0;
  virtual void BindIndexBuffer(BufferHandle handle, uint64_t offset,
                               EFormat format) = 0;
  virtual void BindDescriptorSet(
      uint32_t firstSet, Span<const DescriptorSetHandle> sets,
      Span<const uint32_t> dynamicOffsets,
      EPipelineBindPoint point = EPipelineBindPoint::graphics) = 0;

  virtual void PushConstants(EShaderStage stage, uint32_t offset, uint32_t size,
                             const void *pData) = 0;

  virtual void UpdateBuffer(BufferHandle handle, uint64_t offset,
                            const void *pData, uint64_t size) = 0;
  virtual void CopyBuffer(BufferHandle src, BufferHandle dst,
                          const BufferCopy &region) = 0;
  virtual void Draw(uint32_t vertexCount, uint32_t instanceCount,
                    uint32_t firstVertex, uint32_t firstInstance) = 0;
  virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                           uint32_t firstIndex, int32_t vertexOffset,
                           uint32_t firstInstance) = 0;

  virtual void Transition(TextureHandle handle, EResourceUsage usage,
                          uint32_t layerCount, uint32_t levelCount) = 0;
  virtual void PipelineBarrier(const ImageBarrier &barrier) = 0;
  virtual void CopyImage(TextureHandle src, TextureHandle dst,
                         const ImageCopyRegion &region) = 0;
};
} // namespace avalon::rhi
