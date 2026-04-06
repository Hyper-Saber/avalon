module;
#include <cstddef>
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.rhi:ring_buffer;

import avalon.core;
import :rhi;
import :types;

export namespace avalon::rhi {
class AVALON_RHI_API RingBufferPool final
    : public mem::AutoDestroyable<RingBufferPool>,
      public NonCopyable {
public:
  RingBufferPool(IRhi &rhi, EResourceUsage usage,
                 EMemoryProperty memoryProperty, size_t segmentSize)
      : m_rhi(rhi), m_usage(usage), m_memoryProperty(memoryProperty),
        m_segmentSize(segmentSize) {}

  ~RingBufferPool() {
    if (m_handle.IsValid()) {
      m_rhi.UnmapMemory(m_handle);
      m_rhi.ReleaseBuffer(m_handle);
    }
  }

  bool Initialize() {
    auto alignment =
        m_usage == EResourceUsage::UniformBuffer
            ? m_rhi.GetCapabilities().limits.minUniformBufferOffsetAlignment
            : m_rhi.GetCapabilities().limits.minStorageBufferOffsetAlignment;
    m_alignedSegmentSize = mem::AlignUp(m_segmentSize, alignment);

    size_t totalSize = m_alignedSegmentSize * m_rhi.GetMaxFrameInFlight();

    BufferCreateInfo info{
        .size = totalSize,
        .usage = m_usage,
        .memoryProperty = m_memoryProperty,
    };

    m_handle = m_rhi.CreateBuffer(info);
    if (!m_handle.IsValid()) {
      return false;
    }
    m_mappedPtr = static_cast<uint8_t *>(m_rhi.MapMemory(m_handle));
    return m_mappedPtr != nullptr;
  }

  void ResetPool() {
    m_allocatedSizeInFrame = 0;
    m_frameIndex = m_rhi.GetCurrentFrameIndex();
    m_alignment =
        m_usage == EResourceUsage::UniformBuffer
            ? m_rhi.GetCapabilities().limits.minUniformBufferOffsetAlignment
            : m_rhi.GetCapabilities().limits.minStorageBufferOffsetAlignment;
  }

  BufferAllocation AllocateAligned(size_t size) {
    auto alignedSize = mem::AlignUp(size, m_alignment);

    size_t segmentBase = m_frameIndex * m_alignedSegmentSize;

    AVALON_ASSERT_MSG(m_allocatedSizeInFrame + alignedSize <=
                          m_alignedSegmentSize,
                      "[RingBufferPool]: Not enough space in the ring buffer "
                      "for the allocation!");

    uint32_t finalOffset =
        static_cast<uint32_t>(segmentBase + m_allocatedSizeInFrame);
    m_allocatedSizeInFrame += alignedSize;

    return {
        .pHostAddress = m_mappedPtr + finalOffset,
        .offset = finalOffset,
        .buffer = m_handle,
    };
  }

  auto GetBufferHandle() const { return m_handle; }
  auto GetSegmentSize() const { return m_alignedSegmentSize; }

private:
  IRhi &m_rhi;
  EResourceUsage m_usage;
  EMemoryProperty m_memoryProperty;
  size_t m_segmentSize;
  size_t m_alignedSegmentSize;
  size_t m_allocatedSizeInFrame = 0;
  size_t m_alignment;
  uint32_t m_frameIndex;

  BufferHandle m_handle;
  uint8_t *m_mappedPtr = nullptr;
};

} // namespace avalon::rhi
