module;
#include <cstdint>
#include <utility>
export module avalon.graphics:renderer_types;

import avalon.core;
import avalon.rhi;
import :types;

using namespace avalon::rhi;

export namespace avalon::graphics {

constexpr StringId kOpaquePassName = "OpaquePass"_id;

struct RenderBatch {
  MaterialHandle material;
  uint32_t firstInstance = 0;
  uint32_t instanceCount = 0;
};

struct RenderPacket {
  Array<MeshHandle> meshHandles;
  Array<MaterialInstanceHandle> materialInstances;
  Array<StandardPushConstant> pushConstants;

  Array<Array<uint32_t>> materialOffsets;
  Array<RenderBatch> opaqueBatches;
  Array<RenderBatch> transparentBatches;

  void Clear() {
    meshHandles.Clear();
    materialInstances.Clear();
    pushConstants.Clear();
    materialOffsets.Clear();
    opaqueBatches.Clear();
    transparentBatches.Clear();
  }

  bool IsEmpty() const noexcept { return meshHandles.IsEmpty(); }
};

struct VirtualTextureDesc {
  StringId nameHash{};
  rhi::EResourceUsage usage = rhi::EResourceUsage::None;
  rhi::EFormat format = rhi::EFormat::Undefined;
  rhi::Extent2D extent{};
  rhi::ESampleCount sampleCount = rhi::ESampleCount::SampleCount1x;
  uint32_t layerCount = 1;
  uint32_t mipLevels = 1;
  rhi::ETextureType textureType = rhi::ETextureType::Texture2D;

  HashType GetHash() const noexcept {
    uint64_t sizePacked = (static_cast<uint64_t>(extent.width) << 32) |
                          static_cast<uint64_t>(extent.height);
    HashType h = Hash::Combine(Hash::kOffsetBasis, sizePacked);

    h = Hash::Combine(h, static_cast<uint64_t>(layerCount));

    uint64_t attrPacked = 0;
    using std::to_underlying;

    attrPacked |= (to_underlying(format) & 0x3FFULL);            // 10 bits
    attrPacked |= (to_underlying(sampleCount) & 0x0FULL) << 10;  // 4 bits
    attrPacked |= (to_underlying(usage) & 0xFFFF'FFFFULL) << 14; // 32 bits

    return Hash::Combine(h, attrPacked);
  }

  bool operator==(const VirtualTextureDesc &other) const noexcept {
    return extent.width == other.extent.width &&
           extent.height == other.extent.height && format == other.format &&
           sampleCount == other.sampleCount && usage == other.usage &&
           layerCount == other.layerCount;
  }
};

struct ResourceTimeline {
  uint32_t firstPassIndex = 0xFFFF'FFFF;
  uint32_t lastPassIndex = 0;

  bool isPersistent;
};

struct VirtualResourceHandle {
  Handle<class VirtualResourceTag> handle =
      Handle<VirtualResourceTag>::Invalid();

  static constexpr uint64_t kExternalBit = 1ULL << 63;

  constexpr void MarkExternal() noexcept { handle.id |= kExternalBit; }

  constexpr bool IsExternal() const noexcept {
    return (handle.id & kExternalBit) != 0;
  }

  constexpr bool IsValid() const noexcept {
    return (handle.id & ~kExternalBit) !=
           Handle<VirtualResourceTag>::kInivalidId;
  }

  constexpr uint32_t GetIndex() const noexcept { return handle.GetIndex(); }

  constexpr uint32_t GetVersion() const noexcept {
    return handle.GetGeneration() & 0x7FFF'FFFF;
  }

  constexpr uint32_t GetGenerationRaw() const noexcept {
    return handle.GetGeneration();
  }

  static constexpr auto Create(uint32_t index, uint32_t version)
      -> VirtualResourceHandle {
    return {Handle<VirtualResourceTag>::Create(index, version)};
  }

  auto operator<=>(const VirtualResourceHandle &) const = default;
};

} // namespace avalon::graphics
