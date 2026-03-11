module;
#include "debug/assert.hpp"
#include <cstddef>
#include <type_traits>
export module avalon.core:span;
import :debug;

export namespace avalon {

template <typename T> class Span {
public:
  using element_type = T;
  using pointer = T *;
  using reference = T &;
  using size_type = std::size_t;

  constexpr Span() noexcept : m_data(nullptr), m_size(0) {}
  constexpr Span(pointer ptr, size_type count) noexcept
      : m_data(ptr), m_size(count) {}

  template <size_type N>
  constexpr Span(element_type (&arr)[N]) noexcept : m_data(arr), m_size(N) {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
  constexpr Span(const Span<U> &other) noexcept
      : m_data(other.GetData()), m_size(other.GetSize()) {}

  constexpr reference operator[](size_type index) const {
    return m_data[index];
  }

  constexpr pointer GetData() const noexcept { return m_data; }
  constexpr size_type GetSize() const noexcept { return m_size; }
  constexpr size_type GetSizeType() const noexcept {
    return m_size * sizeof(T);
  }
  constexpr bool IsEmpty() const noexcept { return m_size == 0; }

  constexpr pointer begin() const noexcept { return m_data; }
  constexpr pointer end() const noexcept { return m_data + m_size; }

  constexpr Span<T> Subspan(size_type offset, size_type count = -1) const {
    AVALON_ASSERT(offset <= m_size);
    size_type actualCount =
        (count == static_cast<size_type>(-1)) ? (m_size - offset) : count;
    AVALON_ASSERT(offset + actualCount <= m_size);
    return Span<T>(m_data + offset, actualCount);
  }

private:
  pointer m_data;
  size_type m_size;
};

template <typename T, std::size_t N> Span(T (&)[N]) -> Span<T>;

} // namespace avalon

static_assert(std::is_trivially_copyable_v<avalon::Span<int>>,
              "Span must be trivially copyable for ABI safety");
static_assert(sizeof(avalon::Span<int>) == sizeof(void *) + sizeof(std::size_t),
              "Span size mismatch");
