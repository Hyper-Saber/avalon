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
    m_textureFreeSlots.Reserve(kMaxTextureDescriptor);
    m_samplerFreeSlots.Reserve(kMaxSamplerDescriptor);

    for (uint32_t i = kMaxTextureDescriptor - 1; i > 0; --i)
      m_textureFreeSlots.PushBack(i);
    for (uint32_t i = kMaxSamplerDescriptor - 1; i > 0; --i)
      m_samplerFreeSlots.PushBack(i);

    VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding = kMaterialsBinding,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
        {
            .binding = kTexturesBinding,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = kMaxTextureDescriptor,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
        {
            .binding = kSamplersBinding,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = kMaxSamplerDescriptor,
            .stageFlags = VK_SHADER_STAGE_ALL,
        },
    };

    VkDescriptorBindingFlags bindingFlags[] = {
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo layoutFlags{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(std::size(bindings)),
        .pBindingFlags = bindingFlags};

    VkDescriptorSetLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &layoutFlags,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = static_cast<uint32_t>(std::size(bindingFlags)),
        .pBindings = bindings};

    if (vkCreateDescriptorSetLayout(m_device, &layoutCI, nullptr,
                                    &m_bindlessLayout) != VK_SUCCESS) {
      return false;
    }

    auto allocation = m_descriptorProvider.Allocate(
        m_bindlessLayout, EDescriptorLifetime::Persistent);
    if (!allocation.has_value())
      return false;
    m_bindlessSet = allocation->set;

    UpdateMaterialBufferDescriptor();

    CreateSceneGlobalsLayout();

    return true;
  }

  uint32_t RegisterTexture(TextureHandle handle) override {
    if (!handle.IsValid())
      return 0;

    std::lock_guard lock(m_mutex);
    if (auto *cached = m_textureToSlot.Get(handle))
      return *cached;

    AVALON_ASSERT(!m_textureFreeSlots.IsEmpty());
    uint32_t index = m_textureFreeSlots.GetBack();
    m_textureFreeSlots.PopBack();

    VkImageView view = m_resourceProvider.GetTexture(handle)->imageView;
    UpdateImageDescriptor(index, view);

    m_textureToSlot.Insert(handle, index);
    return index;
  }

  uint32_t RegisterSampler(SamplerHandle handle) override {
    if (!handle.IsValid())
      return 0;

    std::lock_guard lock(m_mutex);
    if (auto *cached = m_samplerToSlot.Get(handle))
      return *cached;

    AVALON_ASSERT(!m_samplerFreeSlots.IsEmpty());
    uint32_t index = m_samplerFreeSlots.GetBack();
    m_samplerFreeSlots.PopBack();

    VkSampler sampler = m_resourceProvider.GetSampler(handle)->sampler;
    UpdateSamplerDescriptor(index, sampler);

    m_samplerToSlot.Insert(handle, index);
    return index;
  }

  void UnregisterTexture(TextureHandle handle) override {
    std::lock_guard lock(m_mutex);
    if (auto *pIndex = m_textureToSlot.Get(handle)) {
      m_pendingDeletions.PushBack(
          {.index = *pIndex,
           .type = ResourceType::View,
           .frameIndex = m_resourceProvider.GetCurrentFrameIndex()});
      m_textureToSlot.Remove(handle);
    }
  }

  void ProcessPendingDeletions() {
    std::lock_guard lock(m_mutex);
    uint64_t completedFrame = m_resourceProvider.GetLastCompletedFrameIndex();

    for (int32_t i = static_cast<int32_t>(m_pendingDeletions.GetSize()) - 1;
         i >= 0; --i) {
      if (m_pendingDeletions[i].frameIndex <= completedFrame) {
        auto &item = m_pendingDeletions[i];
        if (item.type == ResourceType::View)
          m_textureFreeSlots.PushBack(item.index);
        else if (item.type == ResourceType::Sampler)
          m_samplerFreeSlots.PushBack(item.index);

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
  auto GetSceneGlobalSetHandle() const -> DescriptorSetHandle {
    return m_sceneGlobalSetHandle;
  }

  auto GetSceneGlobalSetLayout() const -> VkDescriptorSetLayout {
    return m_sceneGlobalsLayout;
  }

private:
  enum class ResourceType { View, Sampler };
  struct PendingDeletion {
    uint32_t index;
    ResourceType type;
    uint64_t frameIndex;
  };

  void CreateSceneGlobalsLayout() {
    VkDescriptorSetLayoutBinding binding{
        .binding = kSceneGlobalsBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = nullptr};

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

  void UpdateMaterialBufferDescriptor() {
    VkDescriptorBufferInfo bufferInfo =
        m_resourceProvider.GetMaterialBufferInfo();

    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = m_bindlessSet,
                               .dstBinding = 0,
                               .dstArrayElement = 0,
                               .descriptorCount = 1,
                               .descriptorType =
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .pBufferInfo = &bufferInfo};
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  }

  void UpdateImageDescriptor(uint32_t index, VkImageView view) {
    VkDescriptorImageInfo imageInfo{
        .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = m_bindlessSet,
                               .dstBinding = 1,
                               .dstArrayElement = index,
                               .descriptorCount = 1,
                               .descriptorType =
                                   VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                               .pImageInfo = &imageInfo};
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  }

  void UpdateSamplerDescriptor(uint32_t index, VkSampler sampler) {
    VkDescriptorImageInfo samplerInfo{.sampler = sampler};

    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = m_bindlessSet,
                               .dstBinding = 2,
                               .dstArrayElement = index,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                               .pImageInfo = &samplerInfo};
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  }

  VkDevice m_device;
  IRenderResourceProvider &m_resourceProvider;
  DescriptorProvider &m_descriptorProvider;

  VkDescriptorSetLayout m_bindlessLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_bindlessSet{VK_NULL_HANDLE};

  VkDescriptorSetLayout m_sceneGlobalsLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_sceneGlobalsSet{VK_NULL_HANDLE};
  DescriptorSetHandle m_sceneGlobalSetHandle;

  Array<uint32_t> m_textureFreeSlots;
  Array<uint32_t> m_samplerFreeSlots;

  HashMap<TextureHandle, uint32_t> m_textureToSlot;
  HashMap<SamplerHandle, uint32_t> m_samplerToSlot;

  Array<PendingDeletion> m_pendingDeletions;
  std::mutex m_mutex;
};

} // namespace avalon::rhi
