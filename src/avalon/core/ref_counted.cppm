module;
#include <cstdint>
export module avalon.core:ref_counted;
import :platform;

export namespace avalon {
class IRefCounted {
public:
  virtual ~IRefCounted() = default;
  virtual uint32_t AddRef() = 0;
  virtual uint32_t Release() = 0;
};

template <typename T>
concept TRefCounted = requires(T t) {
  { t->AddRef() };
  { t->Release() };
};

template <TRefCounted T> class TRef {
public:
  TRef() : m_ptr(nullptr) {}
  TRef(T *ptr) : m_ptr(ptr) {
    if (m_ptr)
      m_ptr->AddRef();
  }
  TRef(const TRef &other) : TRef(other.m_ptr) {}
  TRef(TRef &&other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }

  ~TRef() { InternalRelease(); }

  TRef &operator=(T *ptr) {
    InternalRelease();
    if (m_ptr != ptr) {
      InternalRelease();
      m_ptr = ptr;
      if (m_ptr)
        m_ptr->AddRef();
    }
    return *this;
  }

  TRef &operator=(const TRef &other) { return *this = other.m_ptr; }
  T operator->() const { return m_ptr; }
  T *Get() const { return m_ptr; }

  T **GetAddressOf() {
    InternalRelease();
    return &m_ptr;
  }
  void **Put() {
    InternalRelease();
    return reinterpret_cast<void **>(&m_ptr);
  }
  explicit operator bool() const { return m_ptr != nullptr; }

private:
  T *m_ptr;
  void InternalRelease() {
    if (m_ptr) {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }
};
} // namespace avalon
