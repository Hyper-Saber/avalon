module;
#include <vector>
module avalon.core;
import :memory;

namespace avalon {
VectorBlob::VectorBlob(std::vector<std::byte> &&data)
    : m_data(std::move(data)) {}
VectorBlob::~VectorBlob() = default;

auto VectorBlob::GetData() const -> const void * { return m_data.data(); }
auto VectorBlob::GetSize() const -> size_t { return m_data.size(); }

} // namespace avalon
