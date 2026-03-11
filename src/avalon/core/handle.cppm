module;
#include <cstdint>
export module avalon.core:handle;

export namespace avalon {
template <typename T> struct Handle {
  uint64_t id = 0;

  static constexpr uint64_t kInivalidId = 0;

  constexpr bool IsValid() const noexcept { return id != kInivalidId; }
  constexpr void Invalidate() noexcept { id = kInivalidId; }

  constexpr uint32_t GetIndex() const noexcept {
    return static_cast<uint32_t>(id & 0xFFFF'FFFF);
  }

  constexpr uint32_t GetGeneration() const noexcept {
    return static_cast<uint32_t>(id >> 32);
  }

  auto operator<=>(const Handle &) const = default;
};
} // namespace avalon
