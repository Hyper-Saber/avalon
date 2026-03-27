module;
export module avalon.ecs:update_world_matrix_system;

import :system;

namespace avalon::ecs {
class UpdateWorldMatrixSystem final
    : public SystemBase<UpdateWorldMatrixSystem> {
  void OnUpdate(class World &world, float dt) override;
};
} // namespace avalon::ecs
