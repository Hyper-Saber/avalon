module;
#include <cstddef>
#include <debug/assert.hpp>
#include <ranges>
#include <utility>
export module avalon.core:containers.hash_map;

import :hash;
import :memory;
import :life_cycle;
import :debug;
import :containers.array;

export namespace avalon {

template <typename K, typename V> struct HashMapEntry {
  alignas(K) char keyData[sizeof(K)];
  alignas(V) char valueData[sizeof(V)];
  bool occupied = false;

  K &GetKey() { return *reinterpret_cast<K *>(keyData); }
  V &GetValue() { return *reinterpret_cast<V *>(valueData); };

  const K &GetKey() const { return *reinterpret_cast<const K *>(keyData); }
  const V &GetValue() const { return *reinterpret_cast<const V *>(valueData); };
};

template <typename K> struct DefaultHasher {
  HashType operator()(const K &key) const noexcept {
    if constexpr (requires { key.GetHash(); }) {
      return static_cast<HashType>(key.GetHash());
    } else {
      return static_cast<HashType>(Hash::Compute(&key, sizeof(K)));
    }
  };
};

template <typename K, typename V, typename Hasher = DefaultHasher<K>>
class HashMap {
public:
  struct Iterator {
    HashMapEntry<K, V> *ptr;
    HashMapEntry<K, V> *endPtr;

    HashMapEntry<K, V> &operator*() const { return *ptr; }
    HashMapEntry<K, V> *operator->() const { return ptr; }

    Iterator &operator++() {
      do {
        ptr++;
      } while (ptr < endPtr && !ptr->occupied);
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator!=(const Iterator &other) const { return ptr != other.ptr; }
  };

  static constexpr float kMaxLoadFactor = 0.6f;
  static constexpr size_t kInitialCapacity = 16;

  HashMap() { Grow(kInitialCapacity); }

  ~HashMap() {
    Clear();
    mem::Allocator<HashMapEntry<K, V>>().Deallocate(m_entries, m_capacity);
  }

  Iterator begin() {
    Iterator it{m_entries, m_entries + m_capacity};
    if (it.ptr < it.endPtr && !it.ptr->occupied)
      it++;
    return it;
  }

  Iterator end() {
    return Iterator{m_entries + m_capacity, m_entries + m_capacity};
  }

  HashMap(const HashMap &other) {
    if (other.m_capacity > 0) {
      m_capacity = other.m_capacity;
      m_entries = static_cast<HashMapEntry<K, V> *>(
          mem::Allocator<HashMapEntry<K, V>>().Allocate(
              m_capacity, mem::MemoryTag::Container));

      for (size_t i = 0; i < m_capacity; ++i) {
        m_entries[i].occupied = false;
      }

      m_size = 0;
      for (size_t i = 0; i < other.m_capacity; ++i) {
        if (other.m_entries[i].occupied) {
          InsertRaw(other.m_entries[i].GetKey(),
                    V(other.m_entries[i].GetValue()));
        }
      }
    }
  }

  HashMap &operator=(HashMap other) noexcept {
    Swap(other);
    return *this;
  }

  HashMap(HashMap &&other) noexcept
      : m_entries(other.m_entries), m_capacity(other.m_capacity),
        m_size(other.m_size) {
    other.m_entries = nullptr;
    other.m_capacity = 0;
    other.m_size = 0;
  }

  HashMap &operator=(HashMap &&other) noexcept {
    if (this != &other) {
      Clear();
      mem::Allocator<HashMapEntry<K, V>>().Deallocate(m_entries, m_capacity);

      m_entries = other.m_entries;
      m_capacity = other.m_capacity;
      m_size = other.m_size;

      other.m_entries = nullptr;
      other.m_capacity = 0;
      other.m_size = 0;
    }
    return *this;
  }

  void Swap(HashMap &other) noexcept {
    std::swap(m_entries, other.m_entries);
    std::swap(m_capacity, other.m_capacity);
    std::swap(m_size, other.m_size);
  }

  V &Insert(const K &key, const V &value) noexcept {
    if (static_cast<float>(m_size + 1) / m_capacity > kMaxLoadFactor) {
      Grow();
    }

    auto index = FindSlot(key);
    if (!m_entries[index].occupied) {
      mem::LifeCycle::Instantiate<K>(&m_entries[index].keyData, key);
      mem::LifeCycle::Instantiate<V>(&m_entries[index].valueData, value);

      m_entries[index].occupied = true;
      m_size++;
    } else {
      m_entries[index].GetValue() = value;
    }

    return m_entries[index].GetValue();
  }

  V &Insert(const K &key, V &&value) noexcept {
    if (static_cast<float>(m_size + 1) / m_capacity > kMaxLoadFactor) {
      Grow();
    }

    auto index = FindSlot(key);
    if (!m_entries[index].occupied) {
      mem::LifeCycle::Instantiate<K>(&m_entries[index].keyData, key);
      mem::LifeCycle::Instantiate<V>(&m_entries[index].valueData,
                                     std::move(value));
      m_entries[index].occupied = true;
      m_size++;
    } else {
      m_entries[index].GetValue() = std::move(value);
    }
    return m_entries[index].GetValue();
  }

  bool Remove(const K &key) noexcept {
    if (m_size == 0)
      return false;

    auto index = FindSlot(key);

    if (!m_entries[index].occupied || !(m_entries[index].GetKey() == key)) {
      return false;
    }

    mem::LifeCycle::Deinstantiate(&m_entries[index].GetValue());
    mem::LifeCycle::Deinstantiate(&m_entries[index].GetKey());
    m_entries[index].occupied = false;
    m_size--;

    RehashAfter(index);

    return true;
  }

  template <typename Func> void RemoveIf(Func &&predicate) {
    for (size_t i = 0; i < m_capacity;)
      if (m_entries[i].occupied && predicate(m_entries[i])) {
        mem::LifeCycle::Deinstantiate(&m_entries[i].GetValue());
        mem::LifeCycle::Deinstantiate(&m_entries[i].GetKey());
        m_entries[i].occupied = false;
        m_size--;

        RehashAfter(i);

      } else {
        i++;
      }
  }

  V *Get(const K &key) const noexcept {
    if (m_capacity == 0)
      return nullptr;

    auto index = FindSlot(key);
    if (m_entries[index].occupied && m_entries[index].GetKey() == key)
      return &m_entries[index].GetValue();
    return nullptr;
  }

  auto GetKeys() const -> Array<K> {
    Array<K> keys;
    keys.Reserve(m_size);

    auto entries_view = std::span(m_entries, m_capacity);
    auto view = entries_view |
                std::views::filter([](const auto &e) { return e.occupied; }) |
                std::views::transform([](const auto &e) { return e.GetKey(); });

    for (const auto &key : view) {
      keys.PushBack(key);
    }
    return keys;
  }

  bool Contains(const K &key) const noexcept { return Get(key) != nullptr; }

  size_t GetSize() const noexcept { return m_size; }

  void Clear() {
    for (size_t i = 0; i < m_capacity; i++) {
      if (m_entries[i].occupied) {
        auto &value = m_entries[i].GetValue();
        auto &key = m_entries[i].GetKey();
        mem::LifeCycle::Deinstantiate(&value);
        mem::LifeCycle::Deinstantiate(&key);

        m_entries[i].occupied = false;
      }
    }
    m_size = 0;
  }

private:
  void RehashAfter(size_t index) noexcept {
    size_t j = index;
    size_t k = (j + 1) & (m_capacity - 1);

    while (m_entries[k].occupied) {
      auto idealIndex = Hasher{}(m_entries[k].GetKey()) & (m_capacity - 1);

      bool shouldMove = false;
      if (k > j) {
        shouldMove = (idealIndex <= j || idealIndex > k);
      } else {
        shouldMove = (idealIndex <= j && idealIndex > k);
      }

      if (shouldMove) {
        mem::LifeCycle::Instantiate<K>(&m_entries[j].keyData,
                                       std::move(m_entries[k].GetKey()));
        mem::LifeCycle::Instantiate<V>(&m_entries[j].valueData,
                                       std::move(m_entries[k].GetValue()));
        m_entries[j].occupied = true;

        mem::LifeCycle::Deinstantiate(&m_entries[k].GetValue());
        mem::LifeCycle::Deinstantiate(&m_entries[k].GetKey());
        m_entries[k].occupied = false;

        j = k;
      }
      k = (k + 1) & (m_capacity - 1);
    }
  }

  size_t FindSlot(const K &key) const noexcept {
    auto hash = Hasher{}(key);
    auto index = hash & (m_capacity - 1);
    while (m_entries[index].occupied && m_entries[index].GetKey() != key) {
      index = (index + 1) & (m_capacity - 1);
    }
    return index;
  }

  void InsertRaw(const K &key, V &&value) {
    auto index = FindSlot(key);
    mem::LifeCycle::Instantiate<K>(&m_entries[index].keyData, key);
    mem::LifeCycle::Instantiate<V>(&m_entries[index].valueData,
                                   std::move(value));
    m_entries[index].occupied = true;
    m_size++;
  }

  void Grow(size_t newCapacity = 0) {
    auto *oldEntries = m_entries;
    auto oldCapacity = m_capacity;
    m_capacity = newCapacity == 0 ? oldCapacity * 2 : newCapacity;

    m_entries = static_cast<HashMapEntry<K, V> *>(
        mem::Allocator<HashMapEntry<K, V>>().Allocate(
            m_capacity, mem::MemoryTag::Container));
    for (size_t i = 0; i < m_capacity; i++) {
      m_entries[i].occupied = false;
    }

    m_size = 0;
    if (oldEntries) {
      for (size_t i = 0; i < oldCapacity; i++) {
        if (oldEntries[i].occupied) {
          InsertRaw(oldEntries[i].GetKey(),
                    std::move(oldEntries[i].GetValue()));
          mem::LifeCycle::Deinstantiate(&oldEntries[i].GetValue());
          mem::LifeCycle::Deinstantiate(&oldEntries[i].GetKey());
        }
      }
      mem::Allocator<HashMapEntry<K, V>>().Deallocate(oldEntries, oldCapacity);
    }
  }

  HashMapEntry<K, V> *m_entries{nullptr};
  size_t m_capacity = 0;
  size_t m_size = 0;
};
} // namespace avalon
