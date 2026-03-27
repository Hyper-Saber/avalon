module;
#include "debug/assert.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>
export module avalon.core:memory.resource_pool;

import :handle;
import :containers.array;
import :memory;
import :debug;
import :life_cycle;
import :string;
import :utils;

export namespace avalon::mem {

template <TAutoDestroyable T> class ResourcePool {
  struct Slot {
    alignas(T) std::byte data[sizeof(T)];
    uint32_t generation = 1;
    bool isActive = false;
    uint32_t nextFreeIndex = 0;
  };

public:
  static constexpr size_t kInitialCapacity = 128;
  ResourcePool(size_t initialCapacity = kInitialCapacity) {
    Grow(initialCapacity);
  }

  ResourcePool(const ResourcePool &) = delete;
  ResourcePool(ResourcePool &&) = delete;
  ResourcePool &operator=(const ResourcePool &) = delete;
  ResourcePool &operator=(ResourcePool &&) = delete;

  ~ResourcePool() { Clear(); }

  template <std::invocable<T &> Func> void Foreach(Func &&func) {
    const uint32_t currentSize = m_slots.GetSize();
    for (uint32_t i = 0; i < currentSize; i++) {
      Slot &slot = m_slots[i];
      if (slot.isActive) {
        T *obj = reinterpret_cast<T *>(slot.data);
        func(*obj);
      }
    }
  }

  template <std::invocable<const T &> Func> void Foreach(Func &&func) const {
    const uint32_t currentSize = m_slots.GetSize();
    for (uint32_t i = 0; i < currentSize; i++) {
      const Slot &slot = m_slots[i];
      if (slot.isActive) {
        const T *obj = reinterpret_cast<T *>(slot.data);
        func(*obj);
      }
    }
  }

  uint32_t GetCapacity() const noexcept { return m_slots.GetCapacity(); }

  template <typename... Args> auto Create(Args &&...args) -> avalon::Handle<T> {
    if (m_freeListHead == UINT32_MAX) {
      Grow();
    }

    uint32_t index = m_freeListHead;
    Slot &slot = m_slots[index];
    m_freeListHead = slot.nextFreeIndex;

    auto obj =
        LifeCycle::Instantiate<T>(slot.data, std::forward<Args>(args)...);
    if (!obj)
      return {};

    slot.isActive = true;

    Handle<T> handle = Handle<T>::Create(index, slot.generation);
    return handle;
  }

  T *Resolve(Handle<T> handle) noexcept {
    AVALON_ASSERT_MSG(handle.IsValid(),
                      String::Format("[ResourcePool<{}>]: Invalid handle!",
                                     utils::GetTypeName<T>()));

    uint32_t index = handle.GetIndex();
    AVALON_ASSERT_MSG(
        index < m_slots.GetSize(),
        String::Format(
            "[ResourcePool<{}>]: Handle index {} out of bounds! Size: {}",
            utils::GetTypeName<T>(), index, m_slots.GetSize()));
    if (index >= m_slots.GetSize())
      return nullptr;
    Slot &slot = m_slots[index];
    bool isValid = slot.isActive && (slot.generation == handle.GetGeneration());

    if (!isValid) {
      const char *reason = !slot.isActive
                               ? "Resource already freed"
                               : "Generation mismatch (stale handle)";

      AVALON_ASSERT_MSG(
          isValid, String::Format("[ResourcePool<{}>]: {}! [Index: {}, "
                                  "HandleGen: {}, SlotGen: {}]",
                                  utils::GetTypeName<T>(), reason, index,
                                  handle.GetGeneration(), slot.generation));
      return nullptr;
    }

    return reinterpret_cast<T *>(slot.data);
  }

  void Release(Handle<T> handle) {
    uint32_t index = handle.GetIndex();
    if (index >= m_slots.GetSize())
      return;
    Slot &slot = m_slots[index];
    if (slot.isActive && slot.generation == handle.GetGeneration()) {
      T *obj = reinterpret_cast<T *>(slot.data);
      LifeCycle::Deinstantiate(obj);
      slot.isActive = false;
      slot.generation++;
      slot.nextFreeIndex = m_freeListHead;
      m_freeListHead = index;
    }
  }

  void Clear() {
    for (uint32_t i = 0; i < m_slots.GetSize(); i++) {
      auto &slot = m_slots[i];
      if (slot.isActive) {
        T *obj = reinterpret_cast<T *>(slot.data);
        LifeCycle::Deinstantiate(obj);
        slot.isActive = false;
        slot.generation++;
        slot.nextFreeIndex = m_freeListHead;
        m_freeListHead = i;
      }
    }
  }

private:
  void Grow(size_t newCapacity = 0) {
    uint32_t oldSize = m_slots.GetSize();
    auto newSize = newCapacity == 0
                       ? (oldSize == 0 ? kInitialCapacity : oldSize * 2)
                       : newCapacity;

    m_slots.ResizeUnInitialized(newSize);

    for (uint32_t i = oldSize; i < newSize; i++) {
      m_slots[i].nextFreeIndex = i + 1;
      m_slots[i].generation = 1;
      m_slots[i].isActive = false;
      m_slots[i].nextFreeIndex = (i == newSize - 1) ? m_freeListHead : i + 1;
      ;
    }
    m_freeListHead = oldSize;
  }

  Array<Slot> m_slots;
  uint32_t m_freeListHead = 0;
};

} // namespace avalon::mem
