module;
#include <concepts>
export module avalon.ecs:system;

import avalon.core;

export namespace avalon::ecs {
class World;

class ISystem : public mem::IAutoDestroyable {
public:
  virtual ~ISystem() = default;
  virtual void OnUpdate(World &world, float dt) = 0;
};

template <typename T>
concept TSystem = std::derived_from<T, ISystem>;

template <typename T> class SystemBase : public ISystem {
public:
  void Destroy() {
    T *pDerived = static_cast<T *>(this);
    pDerived->~T();
    mem::Allocator<T> alloc;
    alloc.Deallocate(pDerived, 1);
  }
};
} // namespace avalon::ecs
