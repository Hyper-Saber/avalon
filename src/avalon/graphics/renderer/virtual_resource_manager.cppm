module;
#include <algorithm>
#include <cstdint>
#include <debug/assert.hpp>
#include <utility>
export module avalon.graphics:virtual_resource_manager;

import :renderer_types;
import avalon.rhi;

namespace avalon::graphics {

class VirtualResourceManager final
    : public NonCopyable,
      public mem::AutoDestroyable<VirtualResourceManager> {
public:
  explicit VirtualResourceManager(rhi::IRhi &rhi) : m_rhi(rhi) {}

  uint32_t ImportExternalTexture(StringId name,
                                 rhi::TextureHandle physicalHandle,
                                 VirtualTextureDesc desc) {
    const auto index = static_cast<uint32_t>(m_virtualDescs.GetSize());
    VirtualResourceHandle handle = VirtualResourceHandle::Create(index, 1);
    m_nameToIndex.Insert(name, index);
    m_vToPMap.Insert(index, physicalHandle);
    handle.MarkExternal();
    m_currentGenerations.PushBack(handle.GetGenerationRaw());
    m_virtualDescs.PushBack(desc);
    return index;
  }

  uint32_t GetOrCreateVirtualResouceIndex(const VirtualTextureDesc &desc) {
    auto pIndex = m_nameToIndex.Get(desc.nameHash);
    if (pIndex != nullptr) {
      auto index = *pIndex;
      auto &exsitingDesc = m_virtualDescs[index];
      AVALON_ASSERT_MSG(
          exsitingDesc.extent == desc.extent,
          String::Format(
              "[RenderGraph]: Dimension mismatch for "
              "the same virtual resource! exsitingDesc: {}, {}, desc: {}, {}",
              exsitingDesc.extent.width, exsitingDesc.extent.height,
              desc.extent.width, desc.extent.height));
      AVALON_ASSERT_MSG(
          exsitingDesc.format == desc.format,
          "[RenderGraph]: Format mismatch for the same virtual resource!");

      if (std::to_underlying(desc.sampleCount) >
          std::to_underlying(exsitingDesc.sampleCount)) {
        exsitingDesc.sampleCount = desc.sampleCount;
      }
      exsitingDesc.usage |= desc.usage;
      return index;
    }

    return CreateNewVirtualResource(desc);
  }

  void RefineUsage(VirtualResourceHandle handle, EResourceUsage usage) {
    auto index = handle.GetIndex();
    AVALON_ASSERT_MSG(
        m_virtualDescs.GetSize() > index,
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    m_virtualDescs[index].usage |= usage;
  }

  auto GetVirtualResource(StringId name) const {
    auto pIndex = m_nameToIndex.Get(name);
    AVALON_ASSERT_MSG(
        pIndex != nullptr,
        String::Format("[RenderGraph]: Virtual resource [{}] not exsit!",
                       name.Resolve()));

    return GetVirtualResource(*pIndex);
  }

  auto GetVirtualResource(uint32_t index) const -> VirtualResourceHandle {
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return VirtualResourceHandle::Create(index, m_currentGenerations[index]);
  }

  auto GetResourceDesc(VirtualResourceHandle handle) const
      -> const VirtualTextureDesc & {
    auto index = handle.GetIndex();
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return m_virtualDescs[index];
  }

  bool IsFirstGeneration(uint32_t index) {
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return m_currentGenerations[index] == 0;
  }

  auto IncreaseGeneration(uint32_t index) -> VirtualResourceHandle {
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    m_currentGenerations[index]++;
    return VirtualResourceHandle::Create(index, m_currentGenerations[index]);
  }

  void RealizeResources(
      const HashMap<VirtualResourceHandle, ResourceTimeline> &timelines) {
    auto sortedHandles = timelines.GetKeys();
    std::ranges::sort(sortedHandles, [&](auto a, auto b) {
      return timelines.Get(a)->firstPassIndex <
             timelines.Get(b)->firstPassIndex;
    });

    auto currentFrame = GetContext().currentFrame;
    for (auto vHandle : sortedHandles) {
      if (vHandle.IsExternal())
        continue;

      const auto &timeline = *timelines.Get(vHandle);
      const auto &vDesc = m_virtualDescs[vHandle.GetIndex()];
      const HashType descHash = vDesc.GetHash();

      bool foundReusable = false;

      if (auto *bucket = m_physicalCache.Get(descHash)) {
        for (auto &entry : *bucket) {
          if (MatchDescriptor(entry.handle, vDesc) &&
              (entry.lastFrameUsed != currentFrame ||
               entry.lastusedPassIndex < timeline.firstPassIndex)) {

            m_vToPMap.Insert(vHandle.GetIndex(), entry.handle);
            entry.lastusedPassIndex = timeline.lastPassIndex;
            entry.lastFrameUsed = currentFrame;
            foundReusable = true;
            break;
          }
        }
      }

      if (!foundReusable) {
        rhi::TextureCreateInfo createInfo{
            .nameHash = vDesc.nameHash,
            .width = vDesc.extent.width,
            .height = vDesc.extent.height,
            .layers = vDesc.layers,
            .format = vDesc.format,
            .usage = vDesc.usage,
            .sampleCount = vDesc.sampleCount,
            .textureType = vDesc.textureType,
        };

        rhi::TextureHandle pHandle = m_rhi.CreateTexture(createInfo);
        m_vToPMap.Insert(vHandle.GetIndex(), pHandle);

        PhysicalEntry newEntry{
            .handle = pHandle,
            .lastusedPassIndex = timeline.lastPassIndex,
            .lastFrameUsed = currentFrame,
        };

        if (auto *bucket = m_physicalCache.Get(descHash)) {
          bucket->PushBack(newEntry);
        } else {
          Array<PhysicalEntry> newBucket;
          newBucket.PushBack(newEntry);
          m_physicalCache.Insert(descHash, std::move(newBucket));
        }
      }
    }

    GarbageCollect();
  }

  rhi::TextureHandle GetPhysical(VirtualResourceHandle handle) const {
    if (auto *pHandle = m_vToPMap.Get(handle.GetIndex())) {
      return *pHandle;
    }
    return rhi::TextureHandle::Invalid();
  }

  void ResetPool() {
    m_virtualDescs.Clear();
    m_currentGenerations.Clear();
    m_nameToIndex.Clear();
    m_vToPMap.Clear();
  }

private:
  bool MatchDescriptor(rhi::TextureHandle physicalTexture,
                       const VirtualTextureDesc &desc) const {
    const auto &physicalCreateInfo =
        m_rhi.GetTextureCreateInfo(physicalTexture);
    if (physicalCreateInfo.width != desc.extent.width ||
        physicalCreateInfo.height != desc.extent.height ||
        physicalCreateInfo.format != desc.format) {
      return false;
    }

    if (physicalCreateInfo.sampleCount != desc.sampleCount) {
      return false;
    }

    if ((physicalCreateInfo.usage & desc.usage) != desc.usage) {
      return false;
    }
    return true;
  }

  auto CreateNewVirtualResource(const VirtualTextureDesc &desc) -> uint32_t {
    AVALON_ASSERT_MSG(desc.format != rhi::EFormat::Undefined,
                      "[RenderGraph]: New resource must have a format!");
    const uint32_t index = static_cast<uint32_t>(m_virtualDescs.GetSize());
    m_virtualDescs.PushBack(desc);
    m_currentGenerations.PushBack(0);
    m_nameToIndex.Insert(desc.nameHash, index);
    return index;
  }

  void GarbageCollect() {
    auto currentFrame = GetContext().currentFrame;
    const uint64_t kThreshold = 120;
    for (auto &entry : m_physicalCache) {
      auto &hash = entry.GetKey();
      auto &bucket = entry.GetValue();
      bucket.RemoveIf([&](const auto &e) {
        if (currentFrame - e.lastFrameUsed > kThreshold) {
          Debug("[Virtual Resource GC] Released. lastFrameUsed: {}, handle: {}",
                e.lastFrameUsed, e.handle.id);
          m_rhi.ReleaseTexture(e.handle);
          return true;
        }
        return false;
      });
    }
  }

  struct PhysicalEntry {
    rhi::TextureHandle handle;
    uint32_t lastusedPassIndex = 0;
    uint64_t lastFrameUsed = 0;
  };

  rhi::IRhi &m_rhi;

  HashMap<HashType, Array<PhysicalEntry>> m_physicalCache;
  Array<VirtualTextureDesc> m_virtualDescs;
  Array<uint32_t> m_currentGenerations;
  HashMap<uint32_t, rhi::TextureHandle> m_vToPMap;
  HashMap<StringId, uint32_t> m_nameToIndex;
};

} // namespace avalon::graphics
