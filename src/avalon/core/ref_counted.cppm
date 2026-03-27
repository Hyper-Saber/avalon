module;
#include <atomic>
#include <concepts>
#include <cstdint>
export module avalon.core:ref_counted;
import :constants;
import :memory;
import :life_cycle;

using namespace avalon::mem;

export namespace avalon {

class IRefCounted : virtual public IAutoDestroyable {
public:
  virtual ~IRefCounted() = default;

  virtual uint32_t AddRef() = 0;

  virtual uint32_t Release() = 0;
};

template <typename T> class RefCounted : public virtual IRefCounted {
public:
  virtual ~RefCounted() = default;

  void *operator new(size_t) = delete;
  void *operator new[](size_t) = delete;

  // for placement new
  void *operator new(size_t, void *ptr) noexcept { return ptr; }

  uint32_t AddRef() noexcept override {
    return m_refCount.fetch_add(1, std::memory_order_relaxed) + 1;
  }

  uint32_t Release() noexcept override {
    uint32_t count = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0) {
      LifeCycle::Destroy(static_cast<T *>(this));
    }
    return count;
  }

protected:
  friend class LifeCycle;

  void Destroy() override {
    T *pDerived = static_cast<T *>(this);
    pDerived->~T();
    Allocator<T> alloc;
    alloc.Deallocate(pDerived, 1);
  }

  virtual bool Initialize() override { return true; }

  RefCounted() noexcept : m_refCount(1) {}
  void operator delete(void *ptr) { ::operator delete(ptr); }

private:
  std::atomic<uint32_t> m_refCount{1};
};

template <typename T>
concept TRefCounted = std::derived_from<T, IRefCounted>;

template <TRefCounted T> class RefCountedPtr {
  template <TRefCounted U> friend class RefCountedPtr;

public:
  enum class InternalTakeOwnership { TakeOwnership };

  RefCountedPtr() : m_ptr(nullptr) {}

  RefCountedPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr)
      m_ptr->AddRef();
  }

  RefCountedPtr(T *ptr, InternalTakeOwnership) noexcept : m_ptr(ptr) {}

  template <TRefCounted U>
    requires std::convertible_to<U *, T *>
  RefCountedPtr(const RefCountedPtr<U> &other) noexcept : m_ptr(other.m_ptr) {
    if (m_ptr)
      m_ptr->AddRef();
  }

  template <TRefCounted U>
    requires std::convertible_to<U *, T *>
  RefCountedPtr(RefCountedPtr<U> &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }

  RefCountedPtr(const RefCountedPtr &other) noexcept
      : RefCountedPtr(other.m_ptr) {}

  RefCountedPtr(RefCountedPtr &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }

  ~RefCountedPtr() { InternalRelease(); }

  RefCountedPtr &operator=(const RefCountedPtr &other) noexcept {
    return *this = other.m_ptr;
  }

  RefCountedPtr &operator=(RefCountedPtr &&other) noexcept {
    if (this != &other) {
      InternalRelease();
      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }

  RefCountedPtr &operator=(T *ptr) noexcept {
    if (m_ptr != ptr) {
      if (ptr)
        ptr->AddRef();
      InternalRelease();
      m_ptr = ptr;
    }
    return *this;
  }

  template <TRefCounted U>
    requires std::convertible_to<U *, T *>
  RefCountedPtr &operator=(const RefCountedPtr<U> &other) noexcept {
    if (reinterpret_cast<void *>(this) != reinterpret_cast<void *>(&other)) {
      if (other.m_ptr)
        other.m_ptr->AddRef();
      InternalRelease();
      m_ptr = other.m_ptr;
    }
    return *this;
  }

  template <TRefCounted U>
    requires std::convertible_to<U *, T *>
  RefCountedPtr &operator=(RefCountedPtr<U> &&other) noexcept {
    if (reinterpret_cast<void *>(this) != reinterpret_cast<void *>(&other)) {
      InternalRelease();

      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }

  T *operator->() const noexcept { return m_ptr; }
  T *Get() const { return m_ptr; }

  T **Put() noexcept {
    InternalRelease();
    m_ptr = nullptr;
    return &m_ptr;
  }

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

private:
  T *m_ptr;
  void InternalRelease() noexcept {
    if (m_ptr) {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }
};

template <TRefCounted T, typename... Args>
RefCountedPtr<T> MakeShared(Args &&...args) {
  Allocator<T> alloc;

  T *pMemory = alloc.Allocate(1);
  if (!pMemory)
    return nullptr;

  T *instance = LifeCycle::Instantiate<T>(pMemory, std::forward<Args>(args)...);

  if (!instance) {
    alloc.Deallocate(pMemory, 1);
    return nullptr;
  }

  RefCountedPtr<T> ptr(instance,
                       RefCountedPtr<T>::InternalTakeOwnership::TakeOwnership);
  return ptr;
}

} // namespace avalon
