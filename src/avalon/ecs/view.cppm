module;
#include <tuple>
export module avalon.ecs:view;

import :types;

export namespace avalon::ecs {
class World;

template <typename... Components> class View {
public:
  explicit View(World &world) : m_world(world) {}

  struct Iterator {
    World &world;
    Entity currentEntity;

    auto operator++() -> Iterator &;

    auto operator*() -> std::tuple<Entity, Components &...> const;

    bool operator!=(const Iterator &other) const;
  };

  Iterator begin();

  Iterator end();

private:
  World &m_world;
};
} // namespace avalon::ecs
