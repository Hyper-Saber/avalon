module;
#include <algorithm>
#include <cstdint>
#include <debug/assert.hpp>
#include <optional>
#include <utility>
export module avalon.graphics:virtual_resource_manager;

import :renderer_types;
import avalon.rhi;

namespace avalon::graphics {

class VirtualResourceManager final
    : public NonCopyable,
      public mem::AutoDestroyable<VirtualResourceManager> {
public:
  explicit VirtualResourceManager(rhi::IRhi &rhi)
      : m_rhi(rhi), m_uboPool(rhi.GetUBOPool()), m_ssboPool(rhi.GetSSBOPool()) {
  }

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
    RefineTextureUsage(handle, desc.usage);
    return index;
  }

  uint32_t ImportExternalBuffer(StringId name, BufferAllocation allocation) {
    auto index = CreateNewVirtualBuffer(name, allocation, true);
    return index;
  }

  uint32_t GetOrCreateVirtualBufferIndex(StringId nameHash,
                                         EResourceUsage usage, uint32_t size) {
    auto pIndex = m_nameToIndex.Get(nameHash);
    if (pIndex != nullptr) {
      auto index = *pIndex;
      return index;
    }

    if (HasFlag(usage, EResourceUsage::SceneGlobals)) {
      AVALON_ASSERT_MSG(

          false,
          "[RenderGraph]: SceneGlobals allocation should have been Imported!")
      return -1;
    }

    if (HasFlag(usage, EResourceUsage::StorageBuffer)) {
      auto allocation = m_ssboPool.AllocateAligned(size);
      return CreateNewVirtualBuffer(nameHash, allocation);
    }

    if (HasFlag(usage, EResourceUsage::UniformBuffer)) {
      auto allocation = m_uboPool.AllocateAligned(size);
      return CreateNewVirtualBuffer(nameHash, allocation);
    }

    AVALON_ASSERT_MSG(
        false,
        String::Format("[RenderGraph]: Unsupported usage: {}", ToView(usage)));
    return -1;
  }

  uint32_t
  GetOrCreateVirtualTextureResouceIndex(const VirtualTextureDesc &desc) {
    auto pIndex = m_nameToIndex.Get(desc.nameHash);
    if (pIndex != nullptr) {
      auto index = *pIndex;
      auto &exsitingDesc = m_virtualDescs[index];

      AVALON_ASSERT_MSG(
          exsitingDesc.extent == desc.extent,
          String::Format(
              "[RenderGraph]: Dimension mismatch for "
              "the same virtual resource! existing: {}x{}, new: {}x{}",
              exsitingDesc.extent.width, exsitingDesc.extent.height,
              desc.extent.width, desc.extent.height));

      AVALON_ASSERT_MSG(
          exsitingDesc.format == desc.format,
          "[RenderGraph]: Format mismatch for the same virtual resource!");

      AVALON_ASSERT_MSG(
          exsitingDesc.layerCount == desc.layerCount,
          String::Format("[RenderGraph]: Layer count mismatch for resource "
                         "'{}'! existing: {}, new: {}",
                         desc.nameHash.Resolve(), exsitingDesc.layerCount,
                         desc.layerCount));

      AVALON_ASSERT_MSG(
          exsitingDesc.mipLevels == desc.mipLevels,
          String::Format("[RenderGraph]: Mipmap level mismatch for resource "
                         "'{}'! existing: {}, new: {}",
                         desc.nameHash.Resolve(), exsitingDesc.mipLevels,
                         desc.mipLevels));

      if (std::to_underlying(desc.sampleCount) >
          std::to_underlying(exsitingDesc.sampleCount)) {
        exsitingDesc.sampleCount = desc.sampleCount;
      }

      exsitingDesc.usage |= desc.usage;

      return index;
    }

    return CreateNewVirtualResource(desc);
  }

  void RefineTextureUsage(VirtualResourceHandle handle, EResourceUsage usage) {
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

    return GetVirtualTexture(*pIndex);
  }

  auto GetVirtualTexture(uint32_t index) const -> VirtualResourceHandle {
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return VirtualResourceHandle::Create(index, m_currentGenerations[index]);
  }

  auto GetVirtualBuffer(StringId nameHash) const -> VirtualResourceHandle {
    auto pIndex = m_nameToIndex.Get(nameHash);
    AVALON_ASSERT_MSG(
        pIndex != nullptr,
        String::Format("[RenderGraph]: Virtual resource [{}] not exsit!",
                       nameHash.Resolve()));

    return GetVirtualBuffer(*pIndex);
  }

  auto GetVirtualBuffer(uint32_t index) const -> VirtualResourceHandle {
    AVALON_ASSERT_MSG(
        index < m_bufferGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return VirtualResourceHandle::Create(index, m_bufferGenerations[index]);
  }

  auto GetTextureDesc(VirtualResourceHandle handle) const
      -> const VirtualTextureDesc & {
    auto index = handle.GetIndex();
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return m_virtualDescs[index];
  }

  bool IsFirstTextureGeneration(uint32_t index) {
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return m_currentGenerations[index] == 0;
  }

  bool IsFirstBufferGeneration(uint32_t index) {
    AVALON_ASSERT_MSG(
        index < m_bufferGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    return m_bufferGenerations[index] == 0;
  }

  auto IncreaseTextureGeneration(uint32_t index) -> VirtualResourceHandle {
    AVALON_ASSERT_MSG(
        index < m_currentGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    m_currentGenerations[index]++;
    return VirtualResourceHandle::Create(index, m_currentGenerations[index]);
  }

  auto IncreaseBufferGeneration(uint32_t index) -> VirtualResourceHandle {
    AVALON_ASSERT_MSG(
        index < m_bufferGenerations.GetSize(),
        String::Format(
            "[RenderGraph]: VirtualResourceHandle index {} not exsit!", index));
    m_bufferGenerations[index]++;
    return VirtualResourceHandle::Create(index, m_bufferGenerations[index]);
  }

  void RealizeTextures(
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
            .layerCount = vDesc.layerCount,
            .mipLevels = vDesc.mipLevels,
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

  rhi::TextureHandle GetPhysicalTexture(VirtualResourceHandle handle) const {
    if (auto *pHandle = m_vToPMap.Get(handle.GetIndex())) {
      return *pHandle;
    }
    return rhi::TextureHandle::Invalid();
  }

  auto GetPhysicalBufferAllocation(VirtualResourceHandle handle) const
      -> std::optional<BufferAllocation> {
    if (auto pAlloc = m_vToPBufferMap.Get(handle.GetIndex())) {
      return *pAlloc;
    }
    return std::nullopt;
  }

  void ResetPool() {
    m_virtualDescs.Clear();
    m_currentGenerations.Clear();
    m_bufferGenerations.Clear();
    m_nameToIndex.Clear();
    m_vToPMap.Clear();
    m_vToPBufferMap.Clear();
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

    if (physicalCreateInfo.layerCount != desc.layerCount) {
      return false;
    }

    if (physicalCreateInfo.mipLevels != desc.mipLevels) {
      return false;
    }

    if ((physicalCreateInfo.usage & desc.usage) != desc.usage) {
      return false;
    }

    return true;
  }

  uint32_t CreateNewVirtualBuffer(StringId nameHash,
                                  BufferAllocation &allocation,
                                  bool isExternal = false) {
    const uint32_t index = static_cast<uint32_t>(m_bufferGenerations.GetSize());
    uint32_t version = 0;
    if (isExternal) {
      auto handle = VirtualResourceHandle::Create(index, version);
      handle.MarkExternal();
      version = handle.GetGenerationRaw();
    }
    m_bufferGenerations.PushBack(version);
    AVALON_ASSERT_MSG(
        !m_nameToIndex.Contains(nameHash),
        String::Format("[RenderGraph]: virtual resource '{}' is already exist!",
                       nameHash.Resolve()));
    m_nameToIndex.Insert(nameHash, index);
    m_vToPBufferMap.Insert(index, allocation);

    return index;
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
  Array<uint32_t> m_bufferGenerations;
  HashMap<uint32_t, rhi::TextureHandle> m_vToPMap;
  HashMap<StringId, uint32_t> m_nameToIndex;
  HashMap<uint32_t, BufferAllocation> m_vToPBufferMap;

  RingBufferPool &m_ssboPool;
  RingBufferPool &m_uboPool;
};

} // namespace avalon::graphics
