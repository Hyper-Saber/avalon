module;
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>
export module avalon.core:memory.blobs;

import :containers.array;
import :unique_ptr;
import :ref_counted;
import :memory;
import :hash;

export namespace avalon {

class IBlob : public IRefCounted {
public:
  virtual ~IBlob() = default;
  virtual auto GetData() const noexcept -> const void * = 0;
  virtual auto GetSize() const noexcept -> size_t = 0;
  virtual auto GetHash() const noexcept -> HashType = 0;

  template <typename T> auto As() const -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }
};

using BlobPtr = UniquePtr<IBlob>;
using SharedBlobPtr = RefCountedPtr<IBlob>;

template <typename T> class BlobBase : public IBlob {
public:
  virtual ~BlobBase() = default;

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
      LifeCycle::Destroy(this);
    }
    return count;
  }

protected:
  friend class LifeCycle;

  friend auto CreateBlob(Array<std::byte> &&data) -> BlobPtr;
  friend auto CreateBlob(const void *data, size_t size) -> BlobPtr;
  friend auto CreateEmptyBlob(size_t size) -> BlobPtr;
  friend auto CreateViewBlob(const void *data, size_t size) -> SharedBlobPtr;

  void Destroy() override {
    T *pDerived = static_cast<T *>(this);
    pDerived->~T();
    Allocator<T> alloc;
    alloc.Deallocate(pDerived, 1);
  }

  virtual bool Initialize() override { return true; }

  BlobBase() noexcept : m_refCount(1) {}
  void operator delete(void *ptr) { ::operator delete(ptr); }

private:
  std::atomic<uint32_t> m_refCount{1};
};

template <TAutoDestroyable T, typename... Args>
  requires std::derived_from<T, IBlob>
auto MakeBlob(Args &&...args) -> BlobPtr {
  return MakeUnique<T>(std::forward<Args>(args)...);
}

template <TRefCounted T, typename... Args>
  requires std::derived_from<T, IBlob>
auto MakeSharedBlob(Args &&...args) -> SharedBlobPtr {
  return MakeShared<T>(std::forward<Args>(args)...);
}

auto AVALON_CORE_API CreateBlob(Array<std::byte> &&data) -> BlobPtr;
auto AVALON_CORE_API CreateBlob(const void *data, size_t size) -> BlobPtr;
auto AVALON_CORE_API CreateEmptyBlob(size_t size = 0) -> BlobPtr;
auto AVALON_CORE_API CreateViewBlob(const void *data, size_t size)
    -> SharedBlobPtr;

} // namespace avalon
