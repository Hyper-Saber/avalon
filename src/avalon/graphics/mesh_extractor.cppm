module;
#include <cstdint>
#include <tuple> // IWYU pragma: keep

export module avalon.graphics:mesh_extractor;

import avalon.ecs;
import avalon.core;
import :mesh;
import :components;

namespace avalon::graphics {

struct RenderPacket {
  Array<MeshHandle> meshHandles;
  Array<Matrix4x4> worldMatrices;
  Array<uint64_t> sortKeys;

  void Clear() {
    meshHandles.Clear();
    worldMatrices.Clear();
    sortKeys.Clear();
  }

  bool IsEmpty() const noexcept { return meshHandles.IsEmpty(); }
};

class MeshExtractor {
public:
  void Extract(ecs::World &world, RenderPacket &outPacket) {

    auto view = world.GetView<ecs::MeshComponent, ecs::TransformComponent>();

    view.ForEach([&](auto entity, auto &meshComp, auto &transComp) {
      outPacket.meshHandles.PushBack(meshComp.meshHandle);
      transComp.UpdateWorldMatrix();
      outPacket.worldMatrices.PushBack(transComp.worldMatrix);
      auto sortKey = static_cast<uint64_t>(meshComp.meshHandle.id) << 32;
      outPacket.sortKeys.PushBack(sortKey);
    });
  }

  void Sort(RenderPacket &packet) {
    if (packet.IsEmpty())
      return;
  }
};

} // namespace avalon::graphics
