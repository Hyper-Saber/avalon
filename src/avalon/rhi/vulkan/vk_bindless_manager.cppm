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
         .stageFlags = VK_SHADER_STAGE_ALL}};

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
        commonFlags                                  // 2D
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

    CreateSceneGlobalsLayout();

    return true;
  }

  uint32_t RegisterTexture(TextureHandle handle) override {
    return RegisterResource(handle, ResourceType::View2D);
  }
  uint32_t RegisterTexture3D(TextureHandle handle) override {
    return RegisterResource(handle, ResourceType::View3D);
  }
  uint32_t RegisterTextureCube(TextureHandle handle) override {
    return RegisterResource(handle, ResourceType::ViewCube);
  }
  uint32_t RegisterTextureArray(TextureHandle handle) override {
    return RegisterResource(handle, ResourceType::ViewArray);
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

  void UnregisterTexture(TextureHandle h) override {
    UnregisterResource(h, ResourceType::View2D);
  }
  void UnregisterTextureCube(TextureHandle h) override {
    UnregisterResource(h, ResourceType::ViewCube);
  }
  void UnregisterTexture3D(TextureHandle h) override {
    UnregisterResource(h, ResourceType::View3D);
  }
  void UnregisterTextureArray(TextureHandle h) override {
    UnregisterResource(h, ResourceType::ViewArray);
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
        case ResourceType::Sampler:
          m_samplerFreeSlots.PushBack(item.index);
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
  enum class ResourceType { View2D, View3D, ViewCube, ViewArray, Sampler };
  struct PendingDeletion {
    uint32_t index;
    ResourceType type;
    uint64_t frameIndex;
  };

  struct ResourcePool {
    uint32_t binding;
    Array<uint32_t> &freeSlots;
    HashMap<TextureHandle, uint32_t> &lookup;
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
    default:
      return {kTexturesBinding, m_texture2DFreeSlots, m_texture2DToSlot};
    }
  }

  uint32_t RegisterResource(TextureHandle handle, ResourceType type) {
    if (!handle.IsValid())
      return 0;
    std::lock_guard lock(m_mutex);

    auto pool = GetPool(type);
    if (auto *cached = pool.lookup.Get(handle))
      return *cached;

    uint32_t index = AllocateSlot(pool.freeSlots);
    VkImageView view = m_resourceProvider.GetTexture(handle)->imageView;

    VkDescriptorImageInfo imageInfo{
        .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    UpdateDescriptor(index, pool.binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                     nullptr, &imageInfo);

    pool.lookup.Insert(handle, index);
    return index;
  }

  void UnregisterResource(TextureHandle handle, ResourceType type) {
    std::lock_guard lock(m_mutex);
    auto pool = GetPool(type);
    if (auto *pIndex = pool.lookup.Get(handle)) {
      m_pendingDeletions.PushBack(
          {.index = *pIndex,
           .type = type,
           .frameIndex = m_resourceProvider.GetCurrentFrameIndex()});
      pool.lookup.Remove(handle);
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
      m_texture2DArrayFreeSlots, m_textureCubeFreeSlots, m_samplerFreeSlots;
  HashMap<TextureHandle, uint32_t> m_texture2DToSlot, m_texture2DArrayToSlot,
      m_texture3DToSlot, m_textureCubeToSlot;
  HashMap<SamplerHandle, uint32_t> m_samplerToSlot;
  Array<PendingDeletion> m_pendingDeletions;
  std::mutex m_mutex;
};

} // namespace avalon::rhi
