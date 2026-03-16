module;
export module avalon.engine:application;

import avalon.core;
import avalon.scene;
import avalon.rhi;

export namespace avalon {

class IApplication : public mem::IAutoDestroyable {
public:
  virtual void OnInitialize(scene::Scene &scene, rhi::IRhi &rhi,
                            rhi::Extent2D) = 0;
  virtual void OnUpdate(float deltaTime, scene::Scene &scene) = 0;
};

template <typename T>
class AVALON_ENGINE_API ApplicationBase : public IApplication {
public:
  void Destroy() {
    T *pDerived = static_cast<T *>(this);
    pDerived->~T();
    mem::Allocator<T> alloc;
    alloc.Deallocate(pDerived, 1);
  }
  virtual void OnUpdate(float deltaTime, scene::Scene &scene) {}
};
} // namespace avalon
