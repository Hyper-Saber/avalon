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
  Array<Transform> transforms;
  Array<uint64_t> sortKeys;

  void Clear() {
    meshHandles.Clear();
    transforms.Clear();
    sortKeys.Clear();
  }

  bool IsEmpty() const noexcept { return meshHandles.IsEmpty(); }
};

class MeshExtractor {
public:
  void Extract(ecs::World &world, RenderPacket &outPacket) {

    auto view = world.GetView<MeshComponent, TransformComponent>();

    for (auto [entity, meshComp, transComp] : view) {
      outPacket.meshHandles.PushBack(meshComp.meshHandle);
      outPacket.transforms.PushBack(transComp.local);
      auto sortKey = static_cast<uint64_t>(meshComp.meshHandle.id) << 32;
      outPacket.sortKeys.PushBack(sortKey);
    }
  }

  void Sort(RenderPacket &packet) {
    if (packet.IsEmpty())
      return;
  }
};

} // namespace avalon::graphics
