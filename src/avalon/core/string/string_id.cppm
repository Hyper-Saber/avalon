module;
#include <type_traits>
export module avalon.core:string_id;

import :hash;
import :constants;
import :string_view;
import :string_registry;

export namespace avalon {

class String;

class StringId {
public:
  constexpr StringId() noexcept : m_id(0) {}
  constexpr explicit StringId(HashType id) noexcept : m_id(id) {}

  constexpr StringId(StringView str) noexcept
      : m_id(Hash::Compute(str.GetData(), str.GetSize())) {

    if (!std::is_constant_evaluated()) {
      if constexpr (debug::kIsDebug) {
        if (str.GetData())
          RegisterStringId(m_id, str);
      }
    }
  }

  constexpr bool operator==(const StringId &other) const noexcept {
    return m_id == other.m_id;
  }

  constexpr bool operator!=(const StringId &other) const noexcept {
    return m_id != other.m_id;
  }

  constexpr bool operator<(const StringId &other) const noexcept {
    return m_id < other.m_id;
  }

  constexpr explicit operator HashType() const noexcept { return m_id; }
  constexpr bool IsValid() const noexcept { return m_id != 0; }

  String Resolve() const;

  constexpr HashType GetHash() const noexcept { return m_id; }

private:
  HashType m_id;
};
} // namespace avalon

export constexpr avalon::StringId operator""_id(const char *pStr,
                                                std::size_t size) {
  return avalon::StringId(avalon::StringView(pStr, size));
}
