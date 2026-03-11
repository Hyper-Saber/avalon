module;
#include <cstddef>
#include <utility>
export module avalon.core:unique_ptr;

import :memory;
import :life_cycle;

using namespace avalon::mem;

export namespace avalon {

template <TAutoDestroyable T> class UniquePtr;

template <typename T, typename... Args> UniquePtr<T> MakeUnique(Args &&...args);

/**
 * @brief 仅支持 TAutoDestroyable 类型。
 * * 核心设计决策：
 * 1. 内存释放职责：由 T->Destroy() 虚函数内部通过 CRTP 结合具体子类的 Allocator
 * 完成。
 * 2. 多态支持：允许 UniquePtr<Base> 持有 Derived*，在 Reset
 * 时通过虚函数分发确保释放 Derived 大小的内存。
 * 3. 约束原因：若 T 不满足
 * TAutoDestroyable，则无法保证在父类指针下正确调用子类分配器的
 * Deallocate(sizeof(Derived))。
 */
template <TAutoDestroyable T> class UniquePtr {

public:
  constexpr UniquePtr() noexcept : m_ptr(nullptr) {}
  constexpr UniquePtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

  template <typename U>
    requires std::convertible_to<U *, T *>
  UniquePtr(UniquePtr<U> &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }

  UniquePtr(const UniquePtr &) = delete;

  UniquePtr(UniquePtr &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }

  ~UniquePtr() { Reset(); }

  UniquePtr &operator=(const UniquePtr &) = delete;

  UniquePtr &operator=(UniquePtr &&other) noexcept {
    if (this != &other) {
      Reset();
      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }

  T *operator->() const noexcept { return m_ptr; }

  T &operator*() const noexcept { return *m_ptr; }

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  T *Get() const noexcept { return m_ptr; }

  T *Release() noexcept {
    T *temp = m_ptr;
    m_ptr = nullptr;
    return temp;
  }

  void Reset() noexcept {
    if (m_ptr) {
      LifeCycle::Destroy(m_ptr);
    }
    m_ptr = nullptr;
  }

private:
  template <TAutoDestroyable U> friend class UniquePtr;

  template <TAutoDestroyable U, typename... Args>
  friend UniquePtr<U> avalon::MakeUnique(Args &&...args);

  explicit UniquePtr(T *ptr) noexcept : m_ptr(ptr) {}

  T *m_ptr{nullptr};
};

template <TAutoDestroyable T, typename... Args>
UniquePtr<T> MakeUnique(Args &&...args) {
  Allocator<T> alloc;
  T *pMemory = alloc.Allocate(1);

  if (!pMemory)
    return nullptr;

  T *instance = LifeCycle::Instantiate<T>(pMemory, std::forward<Args>(args)...);
  if (!instance) {
    alloc.Deallocate(pMemory, 1);
    return nullptr;
  }

  UniquePtr<T> ptr(instance);

  return ptr;
}

} // namespace avalon
