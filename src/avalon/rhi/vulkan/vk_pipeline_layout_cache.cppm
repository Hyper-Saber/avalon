module;
#include <cstdint>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:pipeline_layout_cache;
import avalon.core;
import avalon.rhi;
import :utils;

namespace avalon::rhi {
class PipelineLayoutCache : public NonCopyable,
                            public mem::AutoDestroyable<PipelineLayoutCache> {
public:
  explicit PipelineLayoutCache(VkDevice device) : m_device(device) {}

  ~PipelineLayoutCache() {
    for (auto entry : m_cache) {
      auto layout = entry.GetValue();
      vkDestroyPipelineLayout(m_device, layout, nullptr);
    }
    m_cache.Clear();
  }

  VkPipelineLayout
  GetOrCreateLayout(const Array<VkDescriptorSetLayout> &layouts,
                    const Array<VkPushConstantRange> &pushConstants) {
    auto hash = CaculateHash(layouts, pushConstants);
    if (m_cache.Contains(hash)) {
      return *m_cache.Get(hash);
    }

    auto layout = CreateLayout(layouts, pushConstants);
    if (layout == VK_NULL_HANDLE)
      return layout;
    m_cache.Insert(hash, layout);
    return layout;
  }

private:
  VkDevice m_device;
  HashMap<HashType, VkPipelineLayout> m_cache;

  HashType CaculateHash(const Array<VkDescriptorSetLayout> &layouts,
                        const Array<VkPushConstantRange> &pushConstants) {
    HashType hash = Hash::kOffsetBasis;
    for (const auto &layout : layouts) {
      hash = Hash::Combine(hash, reinterpret_cast<HashType>(layout));
    }
    for (const auto &constant : pushConstants) {
      hash = Hash::Combine(hash, constant.stageFlags);
      hash = Hash::Combine(hash, constant.offset);
      hash = Hash::Combine(hash, constant.size);
    }
    return hash;
  }

  VkPipelineLayout
  CreateLayout(const Array<VkDescriptorSetLayout> &layouts,
               const Array<VkPushConstantRange> &pushConstants) {
    VkPipelineLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(layouts.GetSize()),
        .pSetLayouts = layouts.GetData(),
        .pushConstantRangeCount =
            static_cast<uint32_t>(pushConstants.GetSize()),
        .pPushConstantRanges = pushConstants.GetData(),
    };

    VkPipelineLayout layout{VK_NULL_HANDLE};
    auto result = vkCreatePipelineLayout(m_device, &info, nullptr, &layout);
    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create pipeline layout! Error code: {}.",
            ToView(result));
    }

    return layout;
  }
};
} // namespace avalon::rhi
