module;
#include <cstddef>
#include <debug/assert.hpp>
#include <ranges>
#include <span>
#include <utility>

export module avalon.core:containers.hash_set;

import :hash;
import :memory;
import :life_cycle;
import :debug;
import :containers.array;
import :containers.hash_map;

export namespace avalon {

template <typename K> struct HashSetEntry {
  alignas(K) char keyData[sizeof(K)];
  bool occupied = false;

  K &GetKey() { return *reinterpret_cast<K *>(keyData); }
  const K &GetKey() const { return *reinterpret_cast<const K *>(keyData); }
};

template <typename K, typename Hasher = DefaultHasher<K>> class HashSet {
public:
  static constexpr float kMaxLoadFactor = 0.6f;
  static constexpr size_t kInitialCapacity = 16;

  HashSet() { Grow(kInitialCapacity); }

  ~HashSet() {
    Clear();
    mem::Allocator<HashSetEntry<K>>().Deallocate(m_entries, m_capacity);
  }

  bool Insert(const K &key) noexcept {
    if (static_cast<float>(m_size + 1) / m_capacity > kMaxLoadFactor) {
      Grow();
    }

    auto index = FindSlot(key);
    if (!m_entries[index].occupied) {
      mem::LifeCycle::Instantiate<K>(&m_entries[index].keyData, key);
      m_entries[index].occupied = true;
      m_size++;
      return true;
    }
    return false;
  }

  bool Remove(const K &key) noexcept {
    auto index = FindSlot(key);
    if (m_entries[index].occupied) {
      mem::LifeCycle::Deinstantiate(&m_entries[index].GetKey());
      m_entries[index].occupied = false;
      m_size--;

      RehashFrom(index);
      return true;
    }
    return false;
  }

  bool Contains(const K &key) const noexcept {
    if (m_capacity == 0)
      return false;
    auto index = FindSlot(key);
    return m_entries[index].occupied;
  }

  size_t GetSize() const noexcept { return m_size; }

  void Clear() {
    for (size_t i = 0; i < m_capacity; i++) {
      if (m_entries[i].occupied) {
        mem::LifeCycle::Deinstantiate(&m_entries[i].GetKey());
        m_entries[i].occupied = false;
      }
    }
    m_size = 0;
  }

  auto GetElements() const -> Array<K> {
    Array<K> elements;
    elements.Reserve(m_size);

    auto entries_view = std::span(m_entries, m_capacity);
    auto view = entries_view |
                std::views::filter([](const auto &e) { return e.occupied; }) |
                std::views::transform([](const auto &e) { return e.GetKey(); });

    for (const auto &key : view) {
      elements.PushBack(key);
    }
    return elements;
  }

private:
  size_t FindSlot(const K &key) const noexcept {
    auto hash = Hasher{}(key);
    auto index = hash & (m_capacity - 1);
    while (m_entries[index].occupied && m_entries[index].GetKey() != key) {
      index = (index + 1) & (m_capacity - 1);
    }
    return index;
  }

  void RehashFrom(size_t index) {
    size_t j = index;
    while (true) {
      j = (j + 1) & (m_capacity - 1);
      if (!m_entries[j].occupied)
        break;

      K tempKey = std::move(m_entries[j].GetKey());
      mem::LifeCycle::Deinstantiate(&m_entries[j].GetKey());
      m_entries[j].occupied = false;
      m_size--;
      Insert(tempKey);
    }
  }

  void Grow(size_t newCapacity = 0) {
    auto *oldEntries = m_entries;
    auto oldCapacity = m_capacity;
    m_capacity = newCapacity == 0 ? oldCapacity * 2 : newCapacity;

    m_entries = static_cast<HashSetEntry<K> *>(
        mem::Allocator<HashSetEntry<K>>().Allocate(m_capacity,
                                                   mem::MemoryTag::Container));

    for (size_t i = 0; i < m_capacity; i++)
      m_entries[i].occupied = false;

    m_size = 0;
    if (oldEntries) {
      for (size_t i = 0; i < oldCapacity; i++) {
        if (oldEntries[i].occupied) {
          Insert(oldEntries[i].GetKey());
          mem::LifeCycle::Deinstantiate(&oldEntries[i].GetKey());
        }
      }
      mem::Allocator<HashSetEntry<K>>().Deallocate(oldEntries, oldCapacity);
    }
  }

  HashSetEntry<K> *m_entries{nullptr};
  size_t m_capacity = 0;
  size_t m_size = 0;
};

} // namespace avalon
