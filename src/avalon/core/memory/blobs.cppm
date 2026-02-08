module;
#include <memory>

export module avalon.core:memory.blobs;
import :memory;
import :containers;

export namespace avalon {

class IBlob {
public:
  virtual ~IBlob() = default;

  virtual auto GetData() const -> const void * = 0;
  virtual auto GetSize() const -> size_t = 0;

  template <typename T> auto As() const -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }
};

using BlobPtr = std::unique_ptr<IBlob, void (*)(IBlob *)>;

template <typename T>
concept TBlobImpl = std::derived_from<T, IBlob>;

template <typename TBlobImpl, typename... Args>
auto MakeBlob(Args &&...args) -> BlobPtr {
  auto *instance = new TBlobImpl(std::forward<Args>(args)...);
  return BlobPtr(instance,
                 [](IBlob *p) { delete static_cast<TBlobImpl *>(p); });
}

auto AVALON_CORE_API CreateBlob(Array<std::byte> &&data) -> BlobPtr;
auto AVALON_CORE_API CreateBlob(const void *data, size_t size) -> BlobPtr;
auto AVALON_CORE_API CreateViewBlob(const void *data, size_t size) -> BlobPtr;

} // namespace avalon
