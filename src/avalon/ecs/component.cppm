module;
#include <concepts>
export module avalon.ecs:component;

export namespace avalon::ecs {
class AVALON_ECS_API IComponent {
public:
  virtual ~IComponent();
};

template <typename T>
concept TComponent = std::derived_from<T, IComponent>;
} // namespace avalon::ecs
