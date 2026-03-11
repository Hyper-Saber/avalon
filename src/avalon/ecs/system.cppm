module;
#include <concepts>
export module avalon.ecs:system;

import avalon.core;

export namespace avalon::ecs {
class World;

class ISystem : public mem::IAutoDestroyable {
  virtual ~ISystem() = default;
  virtual void OnUpdate(World &world, float dt) = 0;

  virtual StringView GetName() const = 0;
};

template <typename T>
concept TSystem = std::derived_from<T, ISystem>;

} // namespace avalon::ecs
