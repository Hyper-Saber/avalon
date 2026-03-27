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
    auto materialHandle = graphics::GetMaterialManager().GetDefaultOpaque();
    auto materialInstanceHandle =
        graphics::GetMaterialManager().GetDefaultOpaqueInstance();

    auto meshData = graphics::GetMeshManager().Resolve(meshHandle)->GetData();

    auto aabb = graphics::MeshData::ComputeFullAABB(meshData);
    world
        .AddComponent<ecs::RenderComponent>(entity, meshHandle, materialHandle,
                                            materialInstanceHandle, aabb)
        .AddComponent<ecs::TransformComponent>(entity);

    return entity;
  }
};

} // namespace avalon::scene
