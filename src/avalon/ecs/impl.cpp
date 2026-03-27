module;
module avalon.ecs;

import :world;
import :transform_component;

namespace avalon::ecs {
World::IPool::~IPool() = default;

void UpdateWorldMatrixSystem::OnUpdate(World &world, float dt) {
  auto view = world.GetView<TransformComponent>();
  view.Foreach([&](auto &trans) { trans.UpdateWorldMatrix(); });
}

} // namespace avalon::ecs
