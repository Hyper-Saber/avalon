module;
#include <cstdint>

export module avalon.graphics:render_packet_extractor;

import avalon.ecs;
import avalon.core;
import :mesh;
import :components;
import :utils;
import :renderer_types;

namespace avalon::graphics {

class RenderPacketExtractor {
public:
  void Extract(ecs::World &world, RenderPacket &outPacket) {

    auto view = world.GetView<ecs::RenderComponent, ecs::TransformComponent>();

    view.Foreach([&](auto entity, auto &renderComp, auto &transComp) {
      outPacket.meshHandles.PushBack(renderComp.meshHandle);
      outPacket.materialInstances.PushBack(renderComp.materialInstanceHandle);
      transComp.UpdateWorldMatrix();
      outPacket.pushConstants.PushBack({
          .model = transComp.worldMatrix,
          .normalMatrix = ComputeNormalMatrix(transComp.worldMatrix),
      });
    });
  }

  void Sort(RenderPacket &packet) {
    if (packet.IsEmpty())
      return;
  }
};

} // namespace avalon::graphics
