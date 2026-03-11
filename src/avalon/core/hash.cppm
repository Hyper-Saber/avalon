module;
#include <cstddef>
#include <cstdint>
export module avalon.core:hash;

export namespace avalon {

using HashType = uint64_t;

struct Hash {
  static constexpr uint64_t kOffsetBasis = 0xcbf29ce484222325ull;
  static constexpr uint64_t kPrime = 0x100000001b3ull;

  template <typename T>
    requires(sizeof(T) == 1)
  static constexpr HashType Compute(const T *pData, size_t size) {
    HashType hash = kOffsetBasis;
    for (size_t i = 0; i < size; i++) {
      hash ^= static_cast<uint64_t>(static_cast<uint8_t>(pData[i]));
      hash *= kPrime;
    }
    return hash;
  }

  static HashType Compute(const void *pData, size_t size) {
    return Compute(static_cast<const uint8_t *>(pData), size);
  }

  static constexpr HashType Combine(uint64_t seed, HashType v) {
    return seed ^ (v + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
  }
};
} // namespace avalon

constexpr avalon::HashType operator""_hash(const char *str, size_t len) {
  return avalon::Hash::Compute<char>(str, len);
}
