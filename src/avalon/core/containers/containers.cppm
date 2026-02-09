module;
#include <array>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

export module avalon.core:containers;
import :memory.allocator;

export namespace avalon {
template <typename T, size_t N> using FixedArray = std::array<T, N>;

using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
using wstring =
    std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator<wchar_t>>;

template <typename K, typename V, typename Compare = std::less<K>>
using Map = std::map<K, V, Compare, Allocator<std::pair<const K, V>>>;
template <typename K, typename V, typename Compare = std::less<K>>
using HashMap =
    std::unordered_map<K, V, Compare, Allocator<std::pair<const K, V>>>;

template <typename T, typename Compare = std::less<T>>
using Set = std::set<T, Compare, Allocator<T>>;
template <typename T, typename Hash = std::hash<T>>
using HashSet = std::unordered_set<T, Hash, Allocator<T>>;

//-------------------------------------------------------------------------------

} // namespace avalon
