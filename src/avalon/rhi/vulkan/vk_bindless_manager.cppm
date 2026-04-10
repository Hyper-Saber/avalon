module;
#include <cstdint>
#include <debug/assert.hpp>
#include <iterator>
#include <mutex>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:bindless_manager;

import avalon.core;
import avalon.rhi;
import :types;
import :descriptor_provider;
import :utils;

namespace {
void InitializeFreeSlots(avalon::Array<uint32_t> &slots, uint32_t size) {
  slots.Clear();
  slots.Reserve(size);
  for (uint32_t i = size; i > 0; --i) {
    slots.PushBack(i - 1);
  }
}
} // namespace

namespace avalon::rhi {

export class BindlessManager final
    : public IBindlessManager,
      public NonCopyable,
      public mem::AutoDestroyable<BindlessManager> {
public:
  BindlessManager(VkDevice device, IRenderResourceProvider &provider,
                  DescriptorProvider &descriptorProvider)
      : m_device(device), m_resourceProvider(provider),
        m_descriptorProvider(descriptorProvider) {}

  ~BindlessManager() {
    if (m_bindlessLayout)
      vkDestroyDescriptorSetLayout(m_device, m_bindlessLayout, nullptr);
    if (m_sceneGlobalsLayout)
      vkDestroyDescriptorSetLayout(m_device, m_sceneGlobalsLayout, nullptr);
  }

  bool Initialize() override {
    InitializeFreeSlots(m_samplerFreeSlots, kMaxSamplerDescriptor);
    InitializeFreeSlots(m_texture2DFreeSlots, kMaxTexture2DDescriptor);
    InitializeFreeSlots(m_textureCubeFreeSlots, kMaxTextureCubeDescriptor);
    InitializeFreeSlots(m_texture3DFreeSlots, kMaxTexture3DDescriptor);
    InitializeFreeSlots(m_texture2DArrayFreeSlots, kMaxTextureArrayDescriptor);
    InitializeFreeSlots(m_rwTextureFreeSlots, kMaxRWTextureDescriptor);
    InitializeFreeSlots(m_rwTextureArrayFreeSlots,
                        kMaxRWTextureArrayDescriptor);

    VkDescriptorSetLayoutBinding bindings[] = {
        {.binding = kSamplersBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
         .descriptorCount = kMaxSamplerDescriptor,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {.binding = kMaterialsBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {.binding = kProbesBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {.binding = kTextureCubeBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
         .descriptorCount = kMaxTextureCubeDescriptor,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {.binding = kTexture3DBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
         .descriptorCount = kMaxTexture3DDescriptor,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {.binding = kTexture2DArrayBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
         .descriptorCount = kMaxTextureArrayDescriptor,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {.binding = kTexturesBinding,
         .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
         .descriptorCount = kMaxTexture2DDescriptor,
         .stageFlags = VK_SHADER_STAGE_ALL},
        {
            .binding = kRWTexturesBinding,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = kMaxRWTextureDescriptor,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
        {
            .binding = kGeneralSSBOBinding,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
        {
            .binding = kRWTextureArraysBinding,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = kMaxRWTextureArrayDescriptor,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
    };

    VkDescriptorBindingFlags commonFlags =
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorBindingFlags bindingFlags[] = {
        commonFlags,                                 // Samplers
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, // Materials
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, // Probes
        commonFlags,                                 // Cube
        commonFlags,                                 // 3D
        commonFlags,                                 // Array
        commonFlags,                                 // 2D
        commonFlags,                                 // RW Textures
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, // RW Buffers
        commonFlags,                                 // RW TextureArray
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlags{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(std::size(bindingFlags)),
        .pBindingFlags = bindingFlags};

    VkDescriptorSetLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &layoutFlags,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = static_cast<uint32_t>(std::size(bindings)),
        .pBindings = bindings};

    if (vkCreateDescriptorSetLayout(m_device, &layoutCI, nullptr,
                                    &m_bindlessLayout) != VK_SUCCESS)
      return false;

    auto allocation = m_descriptorProvider.Allocate(
        m_bindlessLayout, EDescriptorLifetime::Persistent);
    if (!allocation.has_value())
      return false;
    m_bindlessSet = allocation->set;

    UpdateMaterialBufferDescriptor();
    UpdateProbeBufferDescriptor();
    UpdateGeneralSSBODescriptor();

    CreateSceneGlobalsLayout();

    return true;
  }

  uint32_t RegisterTexture(TextureHandle handle, EResourceUsage usage,
                           int32_t mipLevel = kNoMiplevels) override {
    return RegisterResource(handle, ResourceType::View2D, usage, mipLevel);
  }
  uint32_t RegisterTexture3D(TextureHandle handle, EResourceUsage usage,
                             int32_t mipLevel = kNoMiplevels) override {
    return RegisterResource(handle, ResourceType::View3D, usage, mipLevel);
  }
  uint32_t RegisterTextureCube(TextureHandle handle, EResourceUsage usage,
                               int32_t mipLevel = kNoMiplevels) override {
    return RegisterResource(handle, ResourceType::ViewCube, usage, mipLevel);
  }
  uint32_t RegisterTextureArray(TextureHandle handle, EResourceUsage usage,
                                int32_t mipLevel = kNoMiplevels) override {
    return RegisterResource(handle, ResourceType::ViewArray, usage, mipLevel);
  }
  uint32_t RegisterRWTexture(TextureHandle handle, EResourceUsage usage,
                             int32_t mipLevel = kNoMiplevels) override {
    return RegisterResource(handle, ResourceType::RWTexture, usage, mipLevel);
  }
  uint32_t RegisterRWTextureArray(TextureHandle handle, EResourceUsage usage,
                                  int32_t mipLevel = kNoMiplevels) override {
    return RegisterResource(handle, ResourceType::RWTextureArray, usage,
                            mipLevel);
  }

  uint32_t RegisterSampler(SamplerHandle handle) override {
    if (!handle.IsValid())
      return 0;
    std::lock_guard lock(m_mutex);
    if (auto *cached = m_samplerToSlot.Get(handle))
      return *cached;

    uint32_t index = AllocateSlot(m_samplerFreeSlots);
    VkSampler sampler = m_resourceProvider.GetSampler(handle)->sampler;
    VkDescriptorImageInfo info{
        .sampler = sampler,
    };

    UpdateDescriptor(index, kSamplersBinding, VK_DESCRIPTOR_TYPE_SAMPLER,
                     nullptr, &info);

    m_samplerToSlot.Insert(handle, index);
    return index;
  }

  void UnregisterTexture(TextureHandle h,
                         int32_t mipLevel = kNoMiplevels) override {
    UnregisterResource(h, ResourceType::View2D, mipLevel);
  }
  void UnregisterTextureCube(TextureHandle h,
                             int32_t mipLevel = kNoMiplevels) override {
    UnregisterResource(h, ResourceType::ViewCube, mipLevel);
  }
  void UnregisterTexture3D(TextureHandle h,
                           int32_t mipLevel = kNoMiplevels) override {
    UnregisterResource(h, ResourceType::View3D, mipLevel);
  }
  void UnregisterTextureArray(TextureHandle h,
                              int32_t miplevel = kNoMiplevels) override {
    UnregisterResource(h, ResourceType::ViewArray, miplevel);
  }
  void UnregisterRWTexture(TextureHandle h,
                           int32_t mipLevel = kNoMiplevels) override {
    UnregisterResource(h, ResourceType::RWTexture, mipLevel);
  }
  void UnregisterRWTextureArray(TextureHandle h,
                                int32_t mipLevel = kNoMiplevels) override {
    UnregisterResource(h, ResourceType::RWTextureArray, mipLevel);
  }

  void ProcessPendingDeletions() {
    std::lock_guard lock(m_mutex);
    uint64_t completedFrame = m_resourceProvider.GetLastCompletedFrameIndex();

    for (int32_t i = static_cast<int32_t>(m_pendingDeletions.GetSize()) - 1;
         i >= 0; --i) {
      auto &item = m_pendingDeletions[i];
      if (item.frameIndex <= completedFrame) {
        switch (item.type) {
        case ResourceType::View2D:
          m_texture2DFreeSlots.PushBack(item.index);
          break;
        case ResourceType::View3D:
          m_texture3DFreeSlots.PushBack(item.index);
          break;
        case ResourceType::ViewCube:
          m_textureCubeFreeSlots.PushBack(item.index);
          break;
        case ResourceType::ViewArray:
          m_texture2DArrayFreeSlots.PushBack(item.index);
          break;
        case ResourceType::RWTexture:
          m_rwTextureFreeSlots.PushBack(item.index);
          break;
        case ResourceType::RWTextureArray:
          m_rwTextureArrayFreeSlots.PushBack(item.index);
          break;
        }
        m_pendingDeletions.RemoveAtSwap(i);
      }
    }
  }

  auto GetBindlessSet() const -> VkDescriptorSet { return m_bindlessSet; }
  auto GetBindlessSetLayout() const -> VkDescriptorSetLayout {
    return m_bindlessLayout;
  }
  auto GetSceneGlobalSet() const -> VkDescriptorSet {
    return m_sceneGlobalsSet;
  }
  auto GetSceneGlobalSetLayout() const -> VkDescriptorSetLayout {
    return m_sceneGlobalsLayout;
  }
  auto GetSceneGlobalSetHandle() const -> DescriptorSetHandle {
    return m_sceneGlobalSetHandle;
  }

private:
  enum class ResourceType {
    View2D,
    View3D,
    ViewCube,
    ViewArray,
    RWTexture,
    RWTextureArray
  };

  struct PendingDeletion {
    uint32_t index;
    ResourceType type;
    uint64_t frameIndex;
  };

  struct ResourceLookupKey {
    TextureHandle handle;
    int32_t mipLevel;

    HashType GetHash() const noexcept {
      uint64_t h = static_cast<uint64_t>(handle.GetIndex());

      uint64_t m = static_cast<uint32_t>(mipLevel);

      uint64_t packed = h | (m << 32);

      return Hash::Combine(Hash::kOffsetBasis, packed);
    }

    bool operator==(const ResourceLookupKey &other) const noexcept {
      return handle == other.handle && mipLevel == other.mipLevel;
    }
  };

  struct ResourcePool {
    uint32_t binding;
    Array<uint32_t> &freeSlots;
    HashMap<ResourceLookupKey, uint32_t> &lookup;
  };

  ResourcePool GetPool(ResourceType type) {
    switch (type) {
    case ResourceType::ViewCube:
      return {kTextureCubeBinding, m_textureCubeFreeSlots, m_textureCubeToSlot};
    case ResourceType::View3D:
      return {kTexture3DBinding, m_texture3DFreeSlots, m_texture3DToSlot};
    case ResourceType::ViewArray:
      return {kTexture2DArrayBinding, m_texture2DArrayFreeSlots,
              m_texture2DArrayToSlot};
    case ResourceType::RWTexture:
      return {kRWTexturesBinding, m_rwTextureFreeSlots, m_rwTextureToSlot};
    case ResourceType::RWTextureArray:
      return {kRWTextureArraysBinding, m_rwTextureArrayFreeSlots,
              m_rwTextureArrayToSlot};
    case ResourceType::View2D:
      return {kTexturesBinding, m_texture2DFreeSlots, m_texture2DToSlot};
    }
  }

  uint32_t RegisterResource(TextureHandle handle, ResourceType type,
                            EResourceUsage usage = EResourceUsage::ReadOnly,
                            int32_t mipLevel = kNoMiplevels) {
    if (!handle.IsValid())
      return 0;
    std::lock_guard lock(m_mutex);

    auto pool = GetPool(type);
    auto key = ResourceLookupKey{handle, mipLevel};
    if (auto *cached = pool.lookup.Get(key))
      return *cached;

    uint32_t index = AllocateSlot(pool.freeSlots);
    auto textureRes = m_resourceProvider.GetTexture(handle);
    VkImageView view = textureRes->imageView;

    if (mipLevel != kNoMiplevels) {
      view = m_resourceProvider.GetOrCreateMipStorageView(
          handle, static_cast<uint32_t>(mipLevel));
    }

    VkDescriptorImageInfo imageInfo{
        .imageView = view,
        .imageLayout = ToVkImageLayout(MapUsageToLayout(usage))};
    UpdateDescriptor(index, pool.binding, MapResourceTypeToDescType(type),
                     nullptr, &imageInfo);

    pool.lookup.Insert(key, index);
    return index;
  }

  VkDescriptorType MapResourceTypeToDescType(ResourceType type) {
    switch (type) {
    case ResourceType::View2D:
    case ResourceType::View3D:
    case ResourceType::ViewCube:
    case ResourceType::ViewArray:
      return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case ResourceType::RWTexture:
    case ResourceType::RWTextureArray:
      return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }
  }

  void UnregisterResource(TextureHandle handle, ResourceType type,
                          int32_t mipLevel = kNoMiplevels) {
    std::lock_guard lock(m_mutex);
    auto pool = GetPool(type);
    auto key = ResourceLookupKey{handle, mipLevel};
    if (auto *pIndex = pool.lookup.Get(key)) {
      m_pendingDeletions.PushBack(
          {.index = *pIndex,
           .type = type,
           .frameIndex = m_resourceProvider.GetCurrentFrameIndex()});
      pool.lookup.Remove(key);
    }
  }

  uint32_t AllocateSlot(Array<uint32_t> &slots) {
    AVALON_ASSERT(!slots.IsEmpty() && "Out of Bindless slots!");
    uint32_t index = slots.GetBack();
    slots.PopBack();
    return index;
  }

  void UpdateDescriptor(uint32_t index, uint32_t binding, VkDescriptorType type,
                        const VkDescriptorBufferInfo *bufferInfo,
                        const VkDescriptorImageInfo *imageInfo) {
    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = m_bindlessSet,
                               .dstBinding = binding,
                               .dstArrayElement = index,
                               .descriptorCount = 1,
                               .descriptorType = type,
                               .pImageInfo = imageInfo,
                               .pBufferInfo = bufferInfo};
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  }

  void UpdateMaterialBufferDescriptor() {
    VkDescriptorBufferInfo info = m_resourceProvider.GetMaterialBufferInfo();
    UpdateDescriptor(0, kMaterialsBinding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     &info, nullptr);
  }

  void UpdateProbeBufferDescriptor() {
    VkDescriptorBufferInfo info = m_resourceProvider.GetProbeBufferInfo();
    UpdateDescriptor(0, kProbesBinding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     &info, nullptr);
  }

  void UpdateGeneralSSBODescriptor() {
    VkDescriptorBufferInfo info = m_resourceProvider.GetGeneralSSBOInfo();
    UpdateDescriptor(0, kGeneralSSBOBinding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     &info, nullptr);
  }

  void CreateSceneGlobalsLayout() {
    VkDescriptorSetLayoutBinding binding{
        .binding = kSceneGlobalsBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL};
    VkDescriptorSetLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding};
    vkCreateDescriptorSetLayout(m_device, &layoutCI, nullptr,
                                &m_sceneGlobalsLayout);

    auto allocation = m_descriptorProvider.Allocate(
        m_sceneGlobalsLayout, EDescriptorLifetime::Persistent);
    m_sceneGlobalsSet = allocation->set;
    m_sceneGlobalSetHandle = {allocation->handle.id};
  }

  VkDevice m_device;
  IRenderResourceProvider &m_resourceProvider;
  DescriptorProvider &m_descriptorProvider;

  VkDescriptorSetLayout m_bindlessLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_bindlessSet{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_sceneGlobalsLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_sceneGlobalsSet{VK_NULL_HANDLE};
  DescriptorSetHandle m_sceneGlobalSetHandle;

  Array<uint32_t> m_texture2DFreeSlots, m_texture3DFreeSlots,
      m_texture2DArrayFreeSlots, m_textureCubeFreeSlots, m_samplerFreeSlots,
      m_rwTextureFreeSlots, m_rwTextureArrayFreeSlots;
  HashMap<ResourceLookupKey, uint32_t> m_texture2DToSlot,
      m_texture2DArrayToSlot, m_texture3DToSlot, m_textureCubeToSlot,
      m_rwTextureToSlot, m_rwTextureArrayToSlot;
  HashMap<SamplerHandle, uint32_t> m_samplerToSlot;
  Array<PendingDeletion> m_pendingDeletions;
  std::mutex m_mutex;
};

} // namespace avalon::rhi
