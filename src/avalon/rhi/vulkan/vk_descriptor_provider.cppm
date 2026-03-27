module;
#include <expected>
#include <iterator>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:descriptor_provider;

import avalon.core;
import :types;

namespace avalon::rhi {

enum class EDescriptorLifetime { Persistent, PerFrame };

struct DescriptorAllocation {
  Handle<DescriptorSetResource> handle;
  VkDescriptorSet set;
  VkDescriptorPool pool;
};

constexpr uint32_t kMaxSampledImageDescriptor = 1024 * 512;
constexpr uint32_t kMaxStorageImageDescriptor = 1024 * 4;
constexpr uint32_t kMaxTextureDescriptor =
    kMaxSampledImageDescriptor + kMaxStorageImageDescriptor;
constexpr uint32_t kMaxSamplerDescriptor = 256;

class DescriptorProvider final
    : public NonCopyable,
      public mem::AutoDestroyable<DescriptorProvider> {
public:
  explicit DescriptorProvider(VkDevice device) : m_device(device) {}

  ~DescriptorProvider() {
    if (m_staticPool) {
      vkDestroyDescriptorPool(m_device, m_staticPool, nullptr);
      m_staticPool = VK_NULL_HANDLE;
    }
    for (auto &pool : m_dynamicPools) {
      if (pool) {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
        pool = VK_NULL_HANDLE;
      }
    }
  }

  bool Initialize() {
    VkDescriptorPoolSize staticSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kMaxTextureDescriptor},
        {VK_DESCRIPTOR_TYPE_SAMPLER, kMaxSamplerDescriptor},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxStorageImageDescriptor},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2048}};

    VkDescriptorPoolCreateInfo staticCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 8,
        .poolSizeCount = static_cast<uint32_t>(std::size(staticSizes)),
        .pPoolSizes = staticSizes};

    if (vkCreateDescriptorPool(m_device, &staticCI, nullptr, &m_staticPool) !=
        VK_SUCCESS) {
      Error("[Vulkan]: Failed to create static descriptor pool!");
      return false;
    }

    VkDescriptorPoolSize dynamicSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128}};

    for (uint32_t i = 0; i < kMaxDynamicPools; ++i) {
      VkDescriptorPoolCreateInfo dynamicCI{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
          .flags = 0,
          .maxSets = 512,
          .poolSizeCount = static_cast<uint32_t>(std::size(dynamicSizes)),
          .pPoolSizes = dynamicSizes};

      auto res = vkCreateDescriptorPool(m_device, &dynamicCI, nullptr,
                                        &m_dynamicPools[i]);
      if (res != VK_SUCCESS) {
        for (uint32_t j = 0; j < i; ++j) {
          vkDestroyDescriptorPool(m_device, m_dynamicPools[j], nullptr);
        }
        Error("[Vulkan]: Failed to create dynamic descriptor pool!");
        return false;
      }
    }

    m_currentFrameIndex = 0;
    return true;
  }

  auto Allocate(VkDescriptorSetLayout layout, EDescriptorLifetime lifetime)
      -> std::expected<DescriptorAllocation, VkResult> {
    VkDescriptorPool targetPool = (lifetime == EDescriptorLifetime::Persistent)
                                      ? m_staticPool
                                      : m_dynamicPools[m_currentFrameIndex];

    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = targetPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout};

    VkDescriptorSet set;
    VkResult res = vkAllocateDescriptorSets(m_device, &allocInfo, &set);

    if (res != VK_SUCCESS) {
      return std::unexpected(res);
    }

    return DescriptorAllocation{.handle = m_resourcePool.Create(set, layout),
                                .set = set,
                                .pool = targetPool};
  }

  auto Resolve(DescriptorSetHandle handle) -> DescriptorSetResource * {
    return m_resourcePool.Resolve({handle.id});
  }

  void Flip() {
    m_currentFrameIndex = (m_currentFrameIndex + 1) % kMaxDynamicPools;
    vkResetDescriptorPool(m_device, m_dynamicPools[m_currentFrameIndex], 0);
  }

private:
  static constexpr uint32_t kMaxDynamicPools = 3;
  VkDevice m_device;
  VkDescriptorPool m_staticPool{VK_NULL_HANDLE};
  VkDescriptorPool m_dynamicPools[kMaxDynamicPools]{};

  mem::ResourcePool<DescriptorSetResource> m_resourcePool;
  uint32_t m_currentFrameIndex{0};
};

} // namespace avalon::rhi
