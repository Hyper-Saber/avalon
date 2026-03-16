module;
#include <expected>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:descriptor_allocator;

import avalon.core;
import avalon.rhi;
import :utils;
import :types;

namespace avalon::rhi {
class DescriptorAllocator final
    : public NonCopyable,
      public mem::AutoDestroyable<DescriptorAllocator> {

public:
  bool Initialize() {
    auto res = CreatePool(1000);
    if (!res.has_value()) {
      return false;
    }
    m_currentPool = res.value();
    return true;
  }

  explicit DescriptorAllocator(VkDevice device) : m_device(device) {}

  ~DescriptorAllocator() {
    if (m_currentPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(m_device, m_currentPool, nullptr);
    }

    for (auto pool : m_usedPools) {
      vkDestroyDescriptorPool(m_device, pool, nullptr);
    }

    m_usedPools.Clear();
  }

  auto Allocate(VkDescriptorSetLayout layout) -> Handle<DescriptorSetResource> {
    VkDescriptorSetAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_currentPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    auto result =
        vkAllocateDescriptorSets(m_device, &allocateInfo, &descriptorSet);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
        result == VK_ERROR_FRAGMENTED_POOL) {
      m_usedPools.PushBack(m_currentPool);

      auto newPoolRes = CreatePool(1000);
      if (!newPoolRes.has_value()) {
        Error("[vulkan]: Failed to create descriptor pool! Error code: {}.",
              ToView(newPoolRes.error()));
        return {};
      }

      m_currentPool = newPoolRes.value();
      allocateInfo.descriptorPool = m_currentPool;

      result =
          vkAllocateDescriptorSets(m_device, &allocateInfo, &descriptorSet);
      if (result != VK_SUCCESS) {
        Error("[vulkan]: Failed to allocate descriptor set! Error code: {}.",
              ToView(result));
        return {};
      }
    }
    return m_descriptorSetPool.Create(descriptorSet, layout);
  }

  auto Resolve(Handle<DescriptorSetResource> handle) {
    return m_descriptorSetPool.Resolve(handle);
  }

  void ResetPools() {
    if (m_currentPool != VK_NULL_HANDLE) {
      m_usedPools.PushBack(m_currentPool);
      m_currentPool = VK_NULL_HANDLE;
    }

    for (auto pool : m_usedPools) {
      vkResetDescriptorPool(m_device, pool, 0);
    }

    if (!m_usedPools.IsEmpty()) {
      m_currentPool = m_usedPools.GetBack();
      m_usedPools.PopBack();
    }

    m_descriptorSetPool.Clear();
  }

private:
  auto CreatePool(uint32_t count) -> std::expected<VkDescriptorPool, VkResult> {
    Array<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count}};

    VkDescriptorPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = count,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.GetSize()),
        .pPoolSizes = poolSizes.GetData(),
    };
    VkDescriptorPool pool;
    auto res = vkCreateDescriptorPool(m_device, &createInfo, nullptr, &pool);
    if (res != VK_SUCCESS) {
      return std::unexpected(res);
    }
    return pool;
  }

  VkDevice m_device;
  VkDescriptorPool m_currentPool{VK_NULL_HANDLE};
  Array<VkDescriptorPool> m_usedPools;
  mem::ResourcePool<DescriptorSetResource> m_descriptorSetPool;
};
} // namespace avalon::rhi
