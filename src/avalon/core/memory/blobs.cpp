module;
#include <bits/move.h>
#include <cstddef>

module avalon.core;
import :memory.blobs;

namespace avalon {

class ArrayBlob final : public IBlob {
public:
  explicit ArrayBlob(Array<std::byte> &&data) : m_data(std::move(data)) {}
  ~ArrayBlob() override = default;

  auto GetData() const -> const void * override { return m_data.data(); }
  auto GetSize() const -> size_t override { return m_data.size(); }

private:
  Array<std::byte> m_data;
};

class ViewBlob final : public IBlob {
public:
  ViewBlob(const void *data, size_t size) : m_ptr(data), m_size(size) {}
  ~ViewBlob() override = default;

  auto GetData() const -> const void * override { return m_ptr; }
  auto GetSize() const -> size_t override { return m_size; }

private:
  const void *m_ptr;
  size_t m_size;
};

class StaticBlob final : public IBlob {
public:
  StaticBlob(const void *src, size_t size) : m_size(size) {
    m_ptr = mem::InternalAlloc(size);
    if (src && m_ptr)
      mem::InternalMemcpy(m_ptr, src, size);
  }
  ~StaticBlob() override { mem::InternalFree(m_ptr, m_size); }

  auto GetData() const -> const void * override { return m_ptr; }
  auto GetSize() const -> size_t override { return m_size; }

private:
  void *m_ptr{nullptr};
  size_t m_size = 0;
};

auto CreateBlob(Array<std::byte> &&data) -> BlobPtr {
  return MakeBlob<ArrayBlob>(std::move(data));
}

auto CreateBlob(const void *data, size_t size) -> BlobPtr {
  return MakeBlob<StaticBlob>(data, size);
}

auto CreateViewBlob(const void *data, size_t size) -> BlobPtr {
  return MakeBlob<ViewBlob>(data, size);
}
} // namespace avalon
