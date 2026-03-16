module;
#include <mutex>
#include <string>
module avalon.core;
import :string;
import :string_view;
import :memory;
import :containers.hash_map;

namespace avalon {
#ifdef AVALON_DEBUG
static HashMap<HashType, String> *gStringRegistry{nullptr};
static std::mutex gStringRegistryMutex;
#endif

void RegisterStringId(HashType id, StringView str) noexcept {
  if constexpr (debug::kIsDebug) {
    std::lock_guard lock(gStringRegistryMutex);
    if (!gStringRegistry) {
      auto pMem = mem::Allocator<HashMap<HashType, String>>().Allocate(
          1, mem::MemoryTag::String);
      gStringRegistry = new (pMem) HashMap<HashType, String>();
    }

    if (!gStringRegistry->Contains(id))
      gStringRegistry->Insert(id, str);
  }
}

String ResolveStringId(HashType id) noexcept {
  if constexpr (debug::kIsDebug) {
    std::lock_guard lock(gStringRegistryMutex);
    if (gStringRegistry && gStringRegistry->Contains(id))
      return *gStringRegistry->Get(id);
    return "Unknown_StringId";

  } else {
    return String(std::to_string(id));
  }
}

} // namespace avalon
