module;
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstring>

module avalon.core;
import :memory;

namespace avalon::mem {
static std::atomic<size_t> g_TotalUsage{0};
static std::atomic<size_t> g_AllocCount{0};
static std::atomic<size_t> g_TotalTransferUsage{0};

AVALON_CORE_API void *InternalAlloc(size_t size, MemoryTag tag) {
  if (size == 0)
    return nullptr;

  void *ptr = std::malloc(size);
  if (ptr) {
    g_TotalUsage.fetch_add(size, std::memory_order_relaxed);
    g_AllocCount.fetch_add(1, std::memory_order_relaxed);
  }
  return ptr;
}

AVALON_CORE_API void InternalFree(void *ptr, size_t size) {
  if (!ptr)
    return;
  free(ptr);
  g_TotalUsage.fetch_sub(size, std::memory_order_relaxed);
  g_AllocCount.fetch_sub(1, std::memory_order_relaxed);
}

AVALON_CORE_API void *InternalMemcpy(void *dest, const void *src, size_t size) {
  if (size == 0)
    return dest;

  g_TotalTransferUsage.fetch_add(size, std::memory_order_relaxed);
  return std::memcpy(dest, src, size);
}

AVALON_CORE_API void *InternalMemset(void *dest, int value, size_t size) {
  if (size == 0)
    return dest;

  g_TotalTransferUsage.fetch_add(size, std::memory_order_relaxed);
  return std::memset(dest, value, size);
}

AVALON_CORE_API auto GetTotalUsage() -> size_t {
  return g_TotalUsage.load(std::memory_order_relaxed);
}
AVALON_CORE_API auto GetAllocCount() -> size_t {
  return g_AllocCount.load(std::memory_order_relaxed);
}

AVALON_CORE_API auto GetTotalTransferUsage() -> size_t {
  return g_TotalTransferUsage.load(std::memory_order_relaxed);
}

} // namespace avalon::mem
