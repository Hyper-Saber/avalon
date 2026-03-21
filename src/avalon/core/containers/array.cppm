module;
#include <algorithm>
#include <cstddef>
#include <cstring>

export module avalon.core:containers.array;
import :memory;
import :life_cycle;

using namespace avalon::mem;

export namespace avalon {

template <typename T> class Array {
public:
  static constexpr size_t kInitialCapacity = 8;
  using Iterator = T *;
  using ConstIterator = const T *;

  Array() noexcept = default;

  ~Array() {
    Clear();
    if (m_data) {
      Allocator<T>().Deallocate(m_data, m_capacity);
    }
  }

  explicit Array(size_t size) {
    if (size > 0) {
      Resize(size);
    }
  }

  Array(std::initializer_list<T> list) {
    if (list.size() > 0) {
      Reserve(list.size());
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(m_data, list.begin(), list.size() * sizeof(T));
        m_size = list.size();
      } else {
        for (const auto &value : list) {
          PushBack(value);
        }
      }
    }
  }

  Array(const T *ptr, size_t size) {
    if (size > 0) {
      Reserve(size);
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(m_data, ptr, size * sizeof(T));
        m_size = size;
      } else {
        for (size_t i = 0; i < size; i++) {
          PushBack(ptr[i]);
        }
      }
    }
  }

  Array(const Array &other)
    requires std::is_copy_constructible_v<T>
  {
    CopyFrom(other);
  }

  Array(const Array &other)
    requires(!std::is_copy_constructible_v<T>)
  = delete;

  Array(Array &&other) noexcept
      : m_data(other.m_data), m_size(other.m_size),
        m_capacity(other.m_capacity) {
    other.m_data = nullptr;
    other.m_size = other.m_capacity = 0;
  }

  Array &operator=(Array &&other) noexcept {
    if (this != &other) {
      Clear();
      if (m_data)
        Allocator<T>().Deallocate(m_data, m_capacity);
      m_data = other.m_data;
      m_size = other.m_size;
      m_capacity = other.m_capacity;
      other.m_data = nullptr;
      other.m_size = other.m_capacity = 0;
    }
    return *this;
  }

  Array &operator=(const Array &other) {
    if (this != &other) {
      Clear();
      if (m_data)
        Allocator<T>().Deallocate(m_data, m_capacity);
      CopyFrom(other);
    }
    return *this;
  }

  Array &operator=(const Array &other)
    requires(!std::is_copy_constructible_v<T>)
  = delete;

  bool operator==(const Array &other) const noexcept {
    if (m_size != other.m_size) {
      return false;
    }

    if (m_data == other.m_data) {
      return true;
    }

    if constexpr (std::is_trivially_copyable_v<T> && std::is_integral_v<T> ||
                  std::is_pointer_v<T>) {
      if (m_size == 0)
        return true;
      return std::memcmp(m_data, other.m_data, m_size * sizeof(T)) == 0;
    } else {
      for (size_t i = 0; i < m_size; ++i) {
        if (!(m_data[i] == other.m_data[i])) {
          return false;
        }
      }
    }
    return true;
  }

  bool operator!=(const Array &other) const noexcept {
    return !(*this == other);
  }

  void PushBack(const T &value) {
    if (m_size == m_capacity)
      Grow(m_capacity == 0 ? kInitialCapacity : m_capacity * 2);
    mem::LifeCycle::Instantiate<T>(&m_data[m_size++], value);
  }

  void PushBack(T &&value) {
    if (m_size == m_capacity)
      Grow(m_capacity == 0 ? kInitialCapacity : m_capacity * 2);
    mem::LifeCycle::Instantiate<T>(&m_data[m_size++], std::move(value));
  }

  void PopBack() { mem::LifeCycle::Deinstantiate<T>(&m_data[--m_size]); }

  void PushBackRaw(const void *pSrc, size_t count) {
    if (count == 0 || pSrc == nullptr)
      return;
    size_t current = m_size;

    if constexpr (std::is_trivially_copyable_v<T>) {
      ResizeUnInitialized(current + count);
      std::memcpy(m_data + current, pSrc, count * sizeof(T));
    } else {
      const T *pSrcElements = static_cast<const T *>(pSrc);
      for (size_t i = 0; i < count; i++) {
        PushBack(pSrcElements[i]);
      }
    }
  }

  template <typename... Args> T &EmplaceBack(Args &&...args) {
    if (m_size == m_capacity)
      Grow(m_capacity == 0 ? kInitialCapacity : m_capacity * 2);

    T *ptr = mem::LifeCycle::Instantiate<T>(&m_data[m_size++],
                                            std::forward<Args>(args)...);
    return *ptr;
  }

  void Resize(size_t newSize) {
    if (newSize > m_capacity) {
      Grow(newSize);
    }

    if (newSize > m_size) {
      for (size_t i = m_size; i < newSize; i++) {
        mem::LifeCycle::Instantiate<T>(&m_data[i]);
      }
    } else if (newSize < m_size) {
      for (size_t i = newSize; i < m_size; i++) {
        mem::LifeCycle::Deinstantiate<T>(&m_data[i]);
      }
    }

    m_size = newSize;
  }

  void ResizeUnInitialized(size_t newSize) {
    if (newSize > m_capacity) {
      Grow(newSize);
    }
    m_size = newSize;
  }

  void Reserve(size_t newCapacity) {
    if (newCapacity > m_capacity)
      Grow(newCapacity);
  }

  void Clear() {
    for (size_t i = 0; i < m_size; i++) {
      mem::LifeCycle::Deinstantiate<T>(&m_data[i]);
    }
    m_size = 0;
  }

  template <typename F> size_t EraseIf(F &&predicate) {
    size_t writeIndex = 0;
    const size_t oldSize = m_size;

    for (size_t readIndex = 0; readIndex < m_size; readIndex++) {
      if (!predicate(m_data[readIndex])) {
        if (readIndex != writeIndex) {
          m_data[writeIndex] = std::move(m_data[readIndex]);
        }
        writeIndex++;
      } else {
        mem::LifeCycle::Deinstantiate<T>(&m_data[readIndex]);
      }
    }
    m_size = writeIndex;
    return oldSize - m_size;
  }

  Iterator begin() noexcept { return m_data; }
  Iterator end() noexcept { return m_data + m_size; }
  ConstIterator begin() const noexcept { return m_data; }
  ConstIterator end() const noexcept { return m_data + m_size; }

  T &operator[](size_t index) { return m_data[index]; }
  const T &operator[](size_t index) const { return m_data[index]; }
  T *GetData() noexcept { return m_data; }
  const T *GetData() const noexcept { return m_data; }
  size_t GetSize() const noexcept { return m_size; }
  size_t GetCapacity() const noexcept { return m_capacity; }
  T &GetFront() const noexcept { return m_data[0]; }
  T &GetBack() const noexcept { return m_data[m_size - 1]; }
  bool IsEmpty() const noexcept { return m_size == 0; }

private:
  void Grow(size_t newCapacity) {
    T *pnewData = Allocator<T>().Allocate(newCapacity);

    if (!pnewData)
      std::abort();

    if (m_data) {
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(pnewData, m_data, m_size * sizeof(T));
      } else {
        for (size_t i = 0; i < m_size; i++) {
          mem::LifeCycle::Instantiate<T>(&pnewData[i], std::move(m_data[i]));
          mem::LifeCycle::Deinstantiate<T>(&m_data[i]);
        }
      }
      Allocator<T>().Deallocate(m_data, m_capacity);
    }

    m_data = pnewData;
    m_capacity = newCapacity;
  }

  void CopyFrom(const Array &other) {
    if (other.m_size == 0) {
      m_data = nullptr;
      m_size = m_capacity = 0;
      return;
    }

    m_data = Allocator<T>().Allocate(other.m_capacity);
    m_capacity = other.m_capacity;
    m_size = other.m_size;

    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(m_data, other.m_data, m_size * sizeof(T));
    } else {
      for (size_t i = 0; i < m_size; i++) {
        mem::LifeCycle::Instantiate<T>(&m_data[i], other.m_data[i]);
      }
    }
  }

  T *m_data = nullptr;
  size_t m_size = 0;
  size_t m_capacity = 0;
};
} // namespace avalon
