module;
#include <type_traits>
#include <typeindex>
#include <utility>

export module avalon.ecs:world;

import avalon.core;
import avalon.rhi;
import :system;
import :types;
import :view;
import :update_world_matrix_system;

export namespace avalon::ecs {

class IRenderDataSink {};

const Entity kNullEntity = 0;

class AVALON_ECS_API World final : public NonCopyable,
                                   public mem::AutoDestroyable<World> {
public:
  World() { AddSystem<UpdateWorldMatrixSystem>(); }
  ~World() = default;

  Entity CreateEntity() { return ++m_entityConuter; }

  template <typename T, typename... Args>
  World &AddComponent(Entity entity, Args &&...args) {
    auto &pool = GetPool<T>();
    pool.Upsert(entity, T(std::forward<Args>(args)...));
    return *this;
  }

  template <typename T> T *GetComponent(Entity entity) {
    auto &pool = GetPool<T>();
    return pool.Contains(entity) ? &pool.Get(entity) : nullptr;
  }

  template <typename T> bool HasComponent(Entity e) {
    return GetPool<T>().Contains(e);
  }

  template <TSystem T, typename... Args> void AddSystem(Args &&...args) {
    auto system = MakeUnique<T>(std::forward<Args>(args)...);
    if constexpr (std::is_base_of_v<IRenderSystem, T>) {
      m_renderSystems.PushBack(std::move(system));
    } else
      m_logicSystems.PushBack(std::move(system));
  }

  auto GetMaxEntity() const { return m_entityConuter; }

  template <typename... T> auto GetView() { return View<T...>(*this); }

  void Update(float dt) {
    for (auto &system : m_logicSystems) {
      system->OnUpdate(*this, dt);
    }
  }

  void Capture(graphics::SceneSnapshot &outSnapshot) {
    for (auto &system : m_renderSystems) {
      system->OnCapture(*this, outSnapshot);
    }
  }

private:
  struct IPool : mem::IAutoDestroyable {
    virtual ~IPool();
  };

  template <typename T> struct ComponentPool final : IPool {
    HashMap<Entity, T> data;

    T &Upsert(const Entity e, T &&value) {
      T *existing = data.Get(e);
      if (existing) {
        *existing = std::move(value);
        return *existing;
      } else {
        return data.Insert(e, std::move(value));
      }
    }

    bool Contains(const Entity e) const { return data.Contains(e); }

    T &Get(const Entity e) { return *data.Get(e); }

    void Destroy() override {
      this->~ComponentPool();
      mem::Allocator<ComponentPool<T>> alloc;
      alloc.Deallocate(this, 1);
    }
  };

  template <typename T> auto &GetPool() {
    auto type = std::type_index(typeid(T));
    if (!m_pools.Contains(type)) {
      m_pools.Insert(type, MakeUnique<ComponentPool<T>>());
    }
    return *static_cast<ComponentPool<T> *>(m_pools.Get(type)->Get());
  };

  Entity m_entityConuter = kNullEntity;
  Array<UniquePtr<ISystem>> m_logicSystems;
  Array<UniquePtr<IRenderSystem>> m_renderSystems;
  HashMap<std::type_index, UniquePtr<IPool>> m_pools;
};

template <typename... Components>
template <typename T>
T &View<Components...>::Get(Entity entity) {
  return *m_world.GetComponent<T>(entity);
}

template <typename... Components>
template <typename T>
const T &View<Components...>::Get(Entity entity) const {
  return *m_world.GetComponent<T>(entity);
}

template <typename... Components>
template <typename Func>
void View<Components...>::Foreach(Func &&func) {
  for (Entity entity : *this) {
    if constexpr (std::is_invocable_v<Func, Entity, Components &...>) {
      func(entity, Get<Components>(entity)...);
    } else {
      func(Get<Components>(entity)...);
    }
  }
}

template <typename... Components>
auto View<Components...>::Iterator::operator++() -> Iterator & {
  while (++currentEntity <= world.GetMaxEntity()) { // 现在 World 是完整的！
    if ((world.HasComponent<Components>(currentEntity) && ...)) {
      break;
    }
  }
  return *this;
}

template <typename... Components>
auto View<Components...>::Iterator::operator*() -> Entity const {
  return currentEntity;
}

template <typename... Components>
bool View<Components...>::Iterator::operator!=(const Iterator &other) const {
  return currentEntity != other.currentEntity;
}

template <typename... Components>
auto View<Components...>::begin() -> Iterator {
  Iterator it{m_world, 0};
  return ++it;
}

template <typename... Components> auto View<Components...>::end() -> Iterator {
  return Iterator{m_world, m_world.GetMaxEntity() + 1};
}

} // namespace avalon::ecs
