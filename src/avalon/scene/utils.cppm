module;
export module avalon.scene:utils;

import avalon.ecs;
import avalon.graphics;

namespace avalon::scene {

class Utils {
public:
  static auto CreatePrimitive(ecs::World &world, graphics::ESDFType type) {
    auto entity = world.CreateEntity();
    auto &meshManager = graphics::GetMeshManager();
    auto &materialManager = graphics::GetMaterialManager();

    auto meshHandle = meshManager.GetDefaultMesh(type);
    meshManager.UploadStandardMesh(meshHandle);
    auto materialHandle = materialManager.GetDefaultOpaque();
    auto materialInstanceHandle = materialManager.GetDefaultOpaqueInstance();

    auto pMesh = meshManager.Resolve(meshHandle);

    auto meshData = pMesh->GetData();

    auto aabb = graphics::MeshData::ComputeFullAABB(meshData);
    world
        .AddComponent<ecs::RenderComponent>(entity, meshHandle, materialHandle,
                                            materialInstanceHandle, aabb, type)
        .AddComponent<ecs::TransformComponent>(entity);

    return entity;
  }
};

} // namespace avalon::scene
