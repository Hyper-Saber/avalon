module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <string_view>
export module avalon.core:string_view;

import :hash;

export namespace avalon {

class StringView {
  static constexpr size_t CalcSize(const char *str) {
    if (!str)
      return 0;
    if (std::is_constant_evaluated()) {
      size_t size = 0;
      while (str[size] != '\0') {
        size++;
      }
      return size;
    }
    return __builtin_strlen(str);
  }

public:
  constexpr static size_t kNpos = -1;
  static const StringView kEmptyView;
  constexpr StringView() noexcept = default;

  constexpr StringView(const char *pStr) noexcept
      : m_pData(pStr), m_size(CalcSize(pStr)) {}

  constexpr StringView(const char *pStr, size_t size) noexcept
      : m_pData(pStr), m_size(size) {}

  template <size_t N>
  constexpr StringView(const char (&str)[N]) noexcept
      : m_pData(str), m_size(N > 0 && str[N - 1] == '\0' ? N - 1 : N) {}

  constexpr const char &operator[](size_t index) const noexcept {
    return m_pData[index];
  }

  constexpr bool operator!=(const StringView &other) const noexcept {
    if (m_size != other.m_size)
      return true;
    return std::memcmp(m_pData, other.m_pData, m_size) != 0;
  }

  constexpr bool operator==(const StringView &other) const noexcept {
    if (m_size != other.m_size)
      return false;
    return std::memcmp(m_pData, other.m_pData, m_size) == 0;
  }

  constexpr bool Contains(const StringView &sub) const noexcept {
    if (sub.IsEmpty())
      return true;
    if (sub.m_size > m_size)
      return false;

    return std::string_view(m_pData, m_size)
               .find(std::string_view(sub.m_pData, sub.m_size)) !=
           std::string_view::npos;
  }

  constexpr bool Contains(const char *pStr) const noexcept {
    return Contains(StringView(pStr));
  }

  constexpr bool IsStartWith(const StringView &prefix) const noexcept {
    if (prefix.IsEmpty())
      return true;
    if (prefix.GetSize() > m_size)
      return false;
    return std::memcmp(m_pData, prefix.GetData(), prefix.GetSize()) == 0;
  }

  constexpr char GetFront() const noexcept { return m_pData[0]; }
  constexpr char GetBack() const noexcept {
    return m_size > 0 ? m_pData[m_size - 1] : '\0';
  }

  constexpr HashType GetHash() const noexcept {
    uint64_t h = Hash::Compute(m_pData, m_size);

    return h;
  }

  constexpr const char *GetData() const noexcept { return m_pData; }
  constexpr size_t GetSize() const noexcept { return m_size; }

  constexpr StringView GetSubView(size_t start) const noexcept {
    return StringView(m_pData + start, m_size - start);
  }

  constexpr bool IsEmpty() const noexcept { return m_size == 0; }

private:
  const char *m_pData = nullptr;
  size_t m_size = 0;
};

const StringView StringView::kEmptyView{};

} // namespace avalon

export namespace std {
template <>
struct formatter<avalon::StringView> : formatter<std::string_view, char> {
  auto format(const avalon::StringView &sv, format_context &ctx) const {
    return formatter<std::string_view, char>::format(
        std::string_view(sv.GetData(), sv.GetSize()), ctx);
  };
};

} // namespace std
