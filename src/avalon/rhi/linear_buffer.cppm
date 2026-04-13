module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
export module avalon.rhi:linear_buffer;

import avalon.core;
import :rhi;
import :types;

export namespace avalon::rhi {
class AVALON_RHI_API LinearBufferPool final
    : public mem::AutoDestroyable<LinearBufferPool>,
      public NonCopyable {
public:
  LinearBufferPool(IRhi &rhi, EResourceUsage usage,
                   EMemoryProperty memoryProperty, size_t size)
      : m_rhi(rhi), m_usage(usage), m_memoryProperty(memoryProperty),
        m_size(size) {}

  ~LinearBufferPool() {
    if (m_handle.IsValid()) {
      m_rhi.UnmapMemory(m_handle);
      m_rhi.ReleaseBuffer(m_handle);
    }
  }

  bool Initialize() {
    m_alignment =
        HasFlag(m_usage, EResourceUsage::UniformBuffer)
            ? m_rhi.GetCapabilities().limits.minUniformBufferOffsetAlignment
            : m_rhi.GetCapabilities().limits.minStorageBufferOffsetAlignment;

    BufferCreateInfo info{
        .size = m_size,
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

  BufferAllocation AllocateAligned(size_t size) {
    auto alignedSize = mem::AlignUp(size, m_alignment);

    AVALON_ASSERT_MSG(m_allocatedSize + alignedSize <= m_size,
                      "LinearBufferPool overflow!");

    BufferAllocation allocation{
        .pHostAddress = m_mappedPtr + m_allocatedSize,
        .buffer = m_handle,
        .offset = static_cast<uint32_t>(m_allocatedSize),
        .size = static_cast<uint32_t>(size),
    };

    m_allocatedSize += alignedSize;

#ifdef AVALON_DEBUG
    m_lastAllocation = allocation;
#endif

    return allocation;
  }

  void UpdateData(uint32_t offset, const void *data, uint32_t size) {
    AVALON_ASSERT(offset + size <= m_size);
    std::memcpy(m_mappedPtr + offset, data, size);
  }

  auto GetBufferHandle() const { return m_handle; }

#ifdef AVALON_DEBUG
  auto DEBUG_GetLastAllocation() -> BufferAllocation & {
    return m_lastAllocation;
  }
#endif

private:
  IRhi &m_rhi;
  EResourceUsage m_usage;
  EMemoryProperty m_memoryProperty;
  size_t m_size;
  size_t m_allocatedSize = 0;
  size_t m_alignment;

  BufferHandle m_handle;
  uint8_t *m_mappedPtr = nullptr;

#ifdef AVALON_DEBUG
  BufferAllocation m_lastAllocation;
#endif
};

} // namespace avalon::rhi
