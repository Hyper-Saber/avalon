module;
#include <cstddef>
#include <debug/assert.hpp>
#include <initializer_list>
#include <type_traits>
export module avalon.core:span;
import :debug;

export namespace avalon {

template <typename T> class Span {
public:
  constexpr Span() noexcept : m_data(nullptr), m_size(0) {}
  constexpr Span(T *ptr, size_t count) noexcept : m_data(ptr), m_size(count) {}

  template <size_t N>
  constexpr Span(T (&arr)[N]) noexcept : m_data(arr), m_size(N) {}

  constexpr Span(std::initializer_list<T> list) noexcept
      : m_data(const_cast<T *>(list.begin())), m_size() {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
  constexpr Span(const Span<U> &other) noexcept
      : m_data(other.GetData()), m_size(other.GetSize()) {}

  constexpr T &operator[](size_t index) const { return m_data[index]; }

  constexpr T *GetData() const noexcept { return m_data; }
  constexpr size_t GetSize() const noexcept { return m_size; }
  constexpr size_t GetSizeType() const noexcept { return m_size * sizeof(T); }
  constexpr bool IsEmpty() const noexcept { return m_size == 0; }

  constexpr T *begin() const noexcept { return m_data; }
  constexpr T *end() const noexcept { return m_data + m_size; }

  constexpr Span<const T> Subspan(size_t offset, size_t count = -1) const {
    AVALON_ASSERT(offset <= m_size);
    size_t actualCount =
        (count == static_cast<size_t>(-1)) ? (m_size - offset) : count;
    AVALON_ASSERT(offset + actualCount <= m_size);
    return Span<T>(m_data + offset, actualCount);
  }

private:
  T *m_data;
  size_t m_size;
};

template <typename T, std::size_t N> Span(T (&)[N]) -> Span<T>;

} // namespace avalon

static_assert(std::is_trivially_copyable_v<avalon::Span<int>>,
              "Span must be trivially copyable for ABI safety");
static_assert(sizeof(avalon::Span<int>) == sizeof(void *) + sizeof(std::size_t),
              "Span size mismatch");
