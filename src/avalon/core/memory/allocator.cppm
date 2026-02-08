module;
#include <cstddef>
export module avalon.core:memory.allocator;

import :memory;

namespace avalon {
template <typename T> class Allocator {
public:
  using value_type = T;

  Allocator() noexcept = default;

  template <typename U> explicit Allocator(const Allocator<U> &) noexcept {}

  T *allocate(size_t n) noexcept {
    return static_cast<T *>(mem::InternalAlloc(n * sizeof(T)));
  }

  void deallocate(T *p, size_t n) noexcept {
    mem::InternalFree(p, n * sizeof(T));
  }

  bool operator==(const Allocator &) const = default;
};
} // namespace avalon
