module;
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstring>

module avalon.core;
import :memory;
import :log;

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
  } else {
    Error("[memory]: Failed to allocate {} bytes.", size);
    std::abort();
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

AVALON_CORE_API auto GetTotalUsage() -> size_t {
  return g_TotalUsage.load(std::memory_order_relaxed);
}
AVALON_CORE_API auto GetAllocCount() -> size_t {
  return g_AllocCount.load(std::memory_order_relaxed);
}

} // namespace avalon::mem
