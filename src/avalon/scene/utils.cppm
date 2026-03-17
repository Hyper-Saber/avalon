module;
export module avalon.scene:utils;

import avalon.ecs;
import avalon.graphics;

namespace avalon::scene {

class Utils {
public:
  static auto CreatePrimitive(ecs::World &world,
                              graphics::EPrimitiveType type) {
    auto entity = world.CreateEntity();
    auto meshHandle = graphics::GetMeshManager().GetDefaultMesh(type);
    world.AddComponent<ecs::MeshComponent>(entity, meshHandle)
        .AddComponent<ecs::TransformComponent>(entity);

    return entity;
  }
};

} // namespace avalon::scene
