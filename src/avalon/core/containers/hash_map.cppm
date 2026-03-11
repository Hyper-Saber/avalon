module;
#include <cstddef>
#include <debug/assert.hpp>
#include <utility>
export module avalon.core:containers.hash_map;

import :hash;
import :memory;
import :life_cycle;
import :debug;

export namespace avalon {

template <typename K, typename V> struct HashMapEntry {
  alignas(K) char keyData[sizeof(K)];
  alignas(V) char valueData[sizeof(V)];
  bool occupied = false;

  K &GetKey() { return *reinterpret_cast<K *>(keyData); }
  V &GetValue() { return *reinterpret_cast<V *>(valueData); };
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

  V *Get(const K &key) const noexcept {
    if (m_capacity == 0)
      return nullptr;

    auto index = FindSlot(key);
    if (m_entries[index].occupied && m_entries[index].GetKey() == key)
      return &m_entries[index].GetValue();
    return nullptr;
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
