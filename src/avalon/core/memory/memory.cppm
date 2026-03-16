module;
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <debug/assert.hpp>

export module avalon.core:memory;

import :debug;

export namespace avalon::mem {

enum class MemoryTag : uint8_t {
  Default,
  Renderer,
  Shader,
  Container,
  Texture,
  String,
};

AVALON_CORE_API void *InternalAlloc(size_t size,
                                    MemoryTag tag = MemoryTag::Default);
AVALON_CORE_API void InternalFree(void *ptr, size_t size);

AVALON_CORE_API auto GetTotalUsage() -> size_t;
AVALON_CORE_API auto GetAllocCount() -> size_t;

template <typename T> class Allocator {
public:
  using value_type = T;
  Allocator() noexcept = default;

  template <typename U> constexpr Allocator(const Allocator<U> &) noexcept {}

  [[nodiscard]] T *
  Allocate(size_t n, mem::MemoryTag tag = mem::MemoryTag::Default) noexcept {
    return static_cast<T *>(mem::InternalAlloc(n * sizeof(T), tag));
  }

  void Deallocate(T *p, size_t n) noexcept {
    mem::InternalFree(p, n * sizeof(T));
  }
};

class AVALON_CORE_API IAutoDestroyable {
public:
  using IsAutoDestroyable = void;
  virtual ~IAutoDestroyable() {
    AVALON_ASSERT(m_lifecycleState == State::Invalidated ||
                  m_lifecycleState == State::Created);
  }

protected:
  friend class LifeCycle;

  virtual void Destroy() = 0;
  virtual bool Initialize() { return true; }

  enum class State { Created, Initialized, Invalidated };
  State m_lifecycleState = State::Created;

  void MarkInitialized() { m_lifecycleState = State::Initialized; }
  void MarkInvalidated() { m_lifecycleState = State::Invalidated; }
};

template <typename T> class AutoDestroyable : public IAutoDestroyable {
protected:
  void Destroy() override {
    T *pDerived = static_cast<T *>(this);
    pDerived->~T();
    mem::Allocator<T> alloc;
    alloc.Deallocate(pDerived, 1);
  }

  virtual bool Initialize() override { return true; }
  friend class LifeCycle;
};

template <typename T>
concept TAutoDestroyable = std::derived_from<T, IAutoDestroyable>;

} // namespace avalon::mem
