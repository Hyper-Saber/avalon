module;
#include <cstddef>
#include <memory>
#include <vector>

export module avalon.core:memory;

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

template <typename TImpl, typename... Args>
auto MakeBlob(Args &&...args) -> BlobPtr {
  auto *instance = new TImpl(std::forward<Args>(args)...);
  return BlobPtr(instance, [](IBlob *p) { delete static_cast<TImpl *>(p); });
}

class VectorBlob : public IBlob {
public:
  explicit VectorBlob(std::vector<std::byte> &&data);
  ~VectorBlob() override;

  auto GetData() const -> const void * override;
  auto GetSize() const -> size_t override;

private:
  std::vector<std::byte> m_data;
};

} // namespace avalon
