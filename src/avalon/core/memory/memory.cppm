module;
#include <cstddef>
#include <cstdint>

export module avalon.core:memory;

export namespace avalon::mem {

enum class MemoryTag : uint8_t {
  Default,
  Renderer,
  Shader,
  Container,
  Texture,
};

AVALON_CORE_API void *InternalAlloc(size_t size,
                                    MemoryTag tag = MemoryTag::Default);
AVALON_CORE_API void InternalFree(void *ptr, size_t size);
AVALON_CORE_API void *InternalMemcpy(void *dest, const void *src, size_t size);
AVALON_CORE_API void *InternalMemset(void *dest, int value, size_t size);

AVALON_CORE_API auto GetTotalUsage() -> size_t;
AVALON_CORE_API auto GetAllocCount() -> size_t;
AVALON_CORE_API auto GetTotalTransferUsage() -> size_t;
} // namespace avalon::mem
