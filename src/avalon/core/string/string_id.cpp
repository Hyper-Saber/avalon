module;
#ifdef AVALON_DEBUG
#include <mutex>
#endif
module avalon.core;
import :string_id;
import :memory;
import :containers.hash_map;

namespace avalon {
#ifdef AVALON_DEBUG
static HashMap<HashType, String> *g_StringRegistry{nullptr};
static std::mutex g_StringRegistryMutex;

void RegisterStringId(HashType id, StringView str) noexcept {
  std::lock_guard lock(g_StringRegistryMutex);
  if (!g_StringRegistry) {
    auto pMem = mem::Allocator<HashMap<HashType, String>>().Allocate(
        1, mem::MemoryTag::String);
    g_StringRegistry = new (pMem) HashMap<HashType, String>();
  }

  if (!g_StringRegistry->Contains(id))
    g_StringRegistry->Insert(id, str);
}

#endif
} // namespace avalon
