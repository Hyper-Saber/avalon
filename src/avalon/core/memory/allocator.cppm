module;
#include <cstddef>

export module avalon.core:memory.allocator;

import :memory;

export namespace avalon {
template <typename T> class Allocator {
public:
  using value_type = T;
  Allocator() noexcept = default;

  template <typename U> constexpr Allocator(const Allocator<U> &) noexcept {}

  [[nodiscard]] T *Allocate(size_t n) noexcept {
    return static_cast<T *>(mem::InternalAlloc(n * sizeof(T)));
  }

  void Deallocate(T *p, size_t n) noexcept {
    mem::InternalFree(p, n * sizeof(T));
  }
};

} // namespace avalon
