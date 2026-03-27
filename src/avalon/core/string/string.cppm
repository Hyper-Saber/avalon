module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <string_view>

export module avalon.core:string;

import :hash;
import :string_id;
import :string_view;
import :memory;

export namespace avalon {

class AVALON_CORE_API String {
public:
  void __ForceDebugSymbolExport() const;

  constexpr String() noexcept
      : m_data{
            .m_small{.buffer = {0}, .state = 0},
        } {}

  String(const char *cstr);
  String(const std::string &stdStr);
  String(StringView view);
  String(const String &other);
  String(String &&other) noexcept;
  ~String();

  String &operator=(const std::string &stdStr);
  String &operator=(const String &other);
  String &operator=(String &&other) noexcept;

  String &Append(StringView other);
  String &Append(char c);

  bool operator==(const String &other) const noexcept {
    const size_t s1 = GetSize();
    const size_t s2 = other.GetSize();

    if (s1 != s2)
      return false;

    if (s1 == 0)
      return true;

    const bool activeIsLarge = IsLarge();
    const bool otherIsLarge = other.IsLarge();

    if (!activeIsLarge && !otherIsLarge) {
      auto *d1 = reinterpret_cast<const uint64_t *>(this);
      auto *d2 = reinterpret_cast<const uint64_t *>(&other);

      return (d1[0] == d2[0]) && (d1[1] == d2[1]) && (d1[2] == d2[2]) &&
             (d1[3] == d2[3]);
    }

    const char *p1 =
        activeIsLarge ? m_data.m_large.pData : m_data.m_small.buffer;
    const char *p2 =
        otherIsLarge ? other.m_data.m_large.pData : other.m_data.m_small.buffer;

    if (p1 == p2)
      return true;

    return std::memcmp(p1, p2, s1) == 0;
  }

  operator StringView() const noexcept {
    return IsLarge() ? StringView(m_data.m_large.pData, m_data.m_large.size)
                     : StringView(m_data.m_small.buffer, GetSmallSize());
  }

  String &operator+=(StringView view) {
    Append(view);
    return *this;
  }

  String &operator+=(const String &other) {
    Append(other);
    return *this;
  }

  String &operator+=(const char *other) {
    Append(other);
    return *this;
  }

  String &operator+=(char c) {
    Append(c);
    return *this;
  }

  const char *GetData() const noexcept {
    return IsLarge() ? m_data.m_large.pData : m_data.m_small.buffer;
  }

  size_t GetSize() const noexcept {
    return IsLarge() ? m_data.m_large.size : GetSmallSize();
  }

  bool IsEmpty() const noexcept { return GetSize() == 0; }

  template <typename... Args>
  static String Format(std::format_string<Args...> format, Args &&...args) {
    std::string s = std::format(format, std::forward<Args>(args)...);
    return String(s);
  }

  StringView GetFinalNameView() const noexcept {
    const size_t size = GetSize();
    if (size == 0)
      return StringView();

    const char *data = GetData();

    for (size_t i = size; i > 0; --i) {
      if (data[i - 1] == '.') {
        return StringView(data + i, size - i);
      }
    }

    return StringView(data, size);
  }

private:
  static constexpr size_t kSSOCapacity = 30;

  AVALON_CORE_API void InitWith(const char *pData, size_t size);
  AVALON_CORE_API void Release() noexcept;

  union DataStorage {
    struct {
      char buffer[kSSOCapacity + 1];
      uint8_t state;
    } m_small;
    struct {
      char *pData;
      size_t size;
      size_t capacity;
      size_t _padding;
    } m_large;
  } m_data;

  bool IsLarge() const noexcept { return m_data.m_small.state & 0x80; }
  uint8_t GetSmallSize() const noexcept { return m_data.m_small.state & 0x7F; }

  void SetSmallState(uint8_t size) noexcept {
    m_data.m_small.state = size & 0x7F;
  }
  void SetLargeFlag() { m_data.m_small.state = 0x80; }
};

inline String operator+(String lhs, StringView rhs) {
  lhs += rhs;
  return lhs;
}

inline String operator+(StringView lhs, const String &rhs) {
  String result = lhs;
  result += rhs;
  return result;
}

inline String operator+(const char *cstr, const String &rhs) {
  String result(cstr);
  result += rhs;
  return result;
}

inline String operator+(String &lhs, const char *cstr) {
  lhs += cstr;
  return lhs;
}

inline String operator+(String &lhs, char c) { return lhs += c; }

} // namespace avalon

static_assert(sizeof(avalon::String) == 32,
              "String class size is not optimized!");

export namespace std {
template <>
struct formatter<avalon::String, char> : formatter<std::string_view, char> {
  auto format(const avalon::String &s, format_context &ctx) const {
    return formatter<std::string_view, char>::format(
        string_view(s.GetData(), s.GetSize()), ctx);
  }
};
} // namespace std
