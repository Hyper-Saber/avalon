module;
export module avalon.ecs:view;

import :types;

export namespace avalon::ecs {
class World;

template <typename... Components> class View {
public:
  explicit View(World &world) : m_world(world) {}
  template <typename T> T &Get(Entity entity);
  template <typename T> T const &Get(Entity entity) const;

  template <typename Func> void Foreach(Func &&func);

  struct Iterator {
    World &world;
    Entity currentEntity;

    auto operator++() -> Iterator &;

    auto operator*() -> Entity const;

    bool operator!=(const Iterator &other) const;
  };

  Iterator begin();
  Iterator end();

private:
  World &m_world;
};
} // namespace avalon::ecs
