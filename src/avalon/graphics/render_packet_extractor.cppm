module;
#include <cstdint>

export module avalon.graphics:render_packet_extractor;

import avalon.ecs;
import avalon.core;
import :mesh;
import :components;
import :utils;

namespace avalon::graphics {

struct PushConstant {
  Matrix4x4 model;
  Matrix4x4 normalMatrix;
};

struct RenderBatch {
  DescriptorSetHandle materialSet;
  uint32_t firstInstance = 0;
  uint32_t instanceCount = 0;
};

struct RenderPacket {
  Array<MeshHandle> meshHandles;
  Array<MaterialInstanceHandle> materialInstances;
  Array<PushConstant> pushConstants;

  Array<Array<uint32_t>> materialOffsets;
  Array<RenderBatch> batches;

  void Clear() {
    meshHandles.Clear();
    materialInstances.Clear();
    pushConstants.Clear();
    materialOffsets.Clear();
    batches.Clear();
  }

  bool IsEmpty() const noexcept { return meshHandles.IsEmpty(); }
};

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
