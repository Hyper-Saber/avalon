module;
#include <cstddef>
#include <cstring>
#include <utility>

module avalon.core;

using namespace avalon::mem;

namespace avalon {

class DataBlob final : public BlobBase<DataBlob> {
public:
  explicit DataBlob(Array<std::byte> &&data) : m_data(std::move(data)) {}
  ~DataBlob() override = default;

  auto GetData() const noexcept -> const void * override {
    return m_data.GetData();
  }
  auto GetSize() const noexcept -> size_t override { return m_data.GetSize(); }
  auto GetHash() const noexcept -> HashType override {
    if (m_cachedHash == 0) {
      m_cachedHash = Hash::Compute(m_data.GetData(), m_data.GetSize());
    }
    return m_cachedHash;
  }

private:
  Array<std::byte> m_data;
  mutable HashType m_cachedHash = 0;
};

class ViewBlob final : public BlobBase<ViewBlob> {
public:
  ViewBlob(const void *data, size_t size) : m_ptr(data), m_size(size) {}
  ~ViewBlob() override = default;

  auto GetData() const noexcept -> const void * override { return m_ptr; }
  auto GetSize() const noexcept -> size_t override { return m_size; }
  auto GetHash() const noexcept -> HashType override {
    if (m_cachedHash == 0) {
      m_cachedHash = Hash::Compute(m_ptr, m_size);
    }
    return m_cachedHash;
  }

private:
  const void *m_ptr;
  size_t m_size;
  mutable HashType m_cachedHash = 0;
};

auto CreateBlob(Array<std::byte> &&data) -> BlobPtr {
  return MakeBlob<DataBlob>(std::move(data));
}

auto CreateBlob(const void *data, size_t size) -> BlobPtr {
  Array<std::byte> array;
  array.PushBackRaw(data, size);
  return MakeBlob<DataBlob>(std::move(array));
}

auto CreateEmptyBlob(size_t size) -> BlobPtr {
  return MakeBlob<DataBlob>(Array<std::byte>(size));
}

auto CreateViewBlob(const void *data, size_t size) -> SharedBlobPtr {
  return MakeSharedBlob<ViewBlob>(data, size);
}
} // namespace avalon
