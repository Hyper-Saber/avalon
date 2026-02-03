module;
#include <vector>

module avalon.core:blobs;
import :memory;

namespace avalon {

class VectorBLob : public IBlob {
public:
  explicit VectorBLob(std::vector<std::byte> &&data)
      : m_data(std::move(data)) {}

  auto GetData() const -> const void * override { return m_data.data(); }
  auto GetSize() const -> size_t override { return m_data.size(); }

private:
  std::vector<std::byte> m_data;
};
} // namespace avalon
