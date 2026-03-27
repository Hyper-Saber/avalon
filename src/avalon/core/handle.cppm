module;
#include <cstdint>
export module avalon.core:handle;

export namespace avalon {
template <typename T> struct Handle {
  uint64_t id = 0;

  static constexpr uint64_t kInivalidId = 0;
  static constexpr uint64_t kInternalId = 0xFFFF'FFFF'FFFF'FFFF;

  static constexpr Handle<T> Create(uint32_t index, uint32_t generation) {
    return {.id = (static_cast<uint64_t>(generation) << 32) | index};
  }

  constexpr bool IsValid() const noexcept {
    return id != kInivalidId && id != kInternalId;
  }

  static constexpr Handle<T> Invalid() noexcept { return {kInivalidId}; }
  static constexpr Handle<T> Internal() noexcept { return {kInternalId}; }

  constexpr void Invalidate() noexcept { id = kInivalidId; }

  constexpr uint32_t GetIndex() const noexcept {
    return static_cast<uint32_t>(id & 0xFFFF'FFFF);
  }

  constexpr uint32_t GetGeneration() const noexcept {
    return static_cast<uint32_t>(id >> 32);
  }

  auto operator<=>(const Handle &) const = default;
};

using ResourceHandle = Handle<void>;
} // namespace avalon
