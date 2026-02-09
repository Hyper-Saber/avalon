module;
#include <algorithm>
#include <cstddef>
export module avalon.core:containers.array;
import :memory.allocator;

export namespace avalon {

template <typename T> class Array {
public:
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
      Reserve(size);
      Resize(size);
    }
  }

  Array(const Array &other) { CopyFrom(other); }

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

  void PushBack(const T &value) {
    if (m_size == m_capacity)
      Grow(m_capacity == 0 ? 8 : m_capacity * 2);
    new (&m_data[m_size++]) T(value);
  }

  void PushBack(T &&value) {
    if (m_size == m_capacity)
      Grow(m_capacity == 0 ? 8 : m_capacity * 2);
    new (&m_data[m_size++]) T(std::move(value));
  }

  template <typename... Args> T &EmplaceBack(Args &&...args) {
    if (m_size == m_capacity)
      Grow(m_capacity == 0 ? 8 : m_capacity * 2);
    T *ptr = new (&m_data[m_size++]) T(std::forward<Args>(args)...);
    return *ptr;
  }

  void Resize(size_t newSize) {
    if (newSize > m_capacity) {
      Grow(newSize);
    }

    if (newSize > m_size) {
      for (size_t i = m_size; i < newSize; i++) {
        new (&m_data[i]) T();
      }
    } else if (newSize < m_size) {
      for (size_t i = newSize; i < m_size; i++) {
        m_data[i].~T();
      }
    }

    m_size = newSize;
  }

  void Reserve(size_t newCapacity) {
    if (newCapacity > m_capacity)
      Grow(newCapacity);
  }

  void Clear() {
    for (size_t i = 0; i < m_size; i++) {
      m_data[i].~T();
    }
    m_size = 0;
  }

  template <typename F> size_t EraseIf(F &&predicate) {
    size_t writeIndex = 0;
    size_t erasedCount = 0;

    for (size_t readIndex = 0; readIndex < m_size; readIndex++) {
      if (predicate(m_data[readIndex])) {
        m_data[writeIndex].~T();
        erasedCount++;
      } else {
        if (readIndex != writeIndex) {
          m_data[writeIndex] = std::move(m_data[readIndex]);
        }
        writeIndex++;
      }
    }

    m_size = writeIndex;
    return erasedCount;
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
  bool IsEmpty() const noexcept { return m_size == 0; }

private:
  void Grow(size_t newCapacity) {
    T *pnewData = Allocator<T>().Allocate(newCapacity);

    if (m_data) {
      for (size_t i = 0; i < m_size; i++) {
        new (&pnewData[i]) T(std::move(m_data[i]));
        m_data[i].~T();
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

    for (size_t i = 0; i < m_size; i++) {
      new (&m_data[i]) T(other.m_data[i]);
    }
  }

  T *m_data = nullptr;
  size_t m_size = 0;
  size_t m_capacity = 0;
};
} // namespace avalon
