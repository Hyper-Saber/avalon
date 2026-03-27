module;
#include <utility>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:descriptor_set_layout_cache;

import avalon.core;
import avalon.rhi;
import :types;
import :utils;

namespace avalon::rhi {

class DescriptorSetLayoutCache final
    : public mem::AutoDestroyable<DescriptorSetLayoutCache> {
public:
  explicit DescriptorSetLayoutCache(VkDevice device) : m_device(device) {}

  ~DescriptorSetLayoutCache() {
    if (m_emptyLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(m_device, m_emptyLayout, nullptr);
      m_emptyLayout = VK_NULL_HANDLE;
    }
    m_cache.Clear();
  }

  auto GetEmptyLayout() -> VkDescriptorSetLayout {
    if (m_emptyLayout != VK_NULL_HANDLE) {
      return m_emptyLayout;
    }

    VkDescriptorSetLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 0,
        .pBindings = nullptr,
    };

    auto res =
        vkCreateDescriptorSetLayout(m_device, &info, nullptr, &m_emptyLayout);
    if (res != VK_SUCCESS) {
      Error("[Vulkan RHI]: Failed to create empty descriptor set layout!");
      return VK_NULL_HANDLE;
    }

    return m_emptyLayout;
  }

  auto GetOrCreate(Span<const DescriptorSetLayoutBinding> bindings,
                   Span<const VkDescriptorSetLayoutBinding> &outBindings)
      -> VkDescriptorSetLayout {
    if (bindings.IsEmpty())
      return GetEmptyLayout();

    HashType hash = Hash::kOffsetBasis;
    for (const auto &b : bindings) {
      hash = Hash::Combine(hash, b.GetHash());
    }

    if (auto cached = m_cache.Get(hash)) {
      auto &res = *cached;
      outBindings = res->GetBindings();
      return res->setLayout;
    }

    auto layout = Create(bindings);
    if (layout) {
      outBindings = layout->GetBindings();
      auto ret = layout->setLayout;
      m_cache.Insert(hash, std::move(layout));
      return ret;
    }

    return VK_NULL_HANDLE;
  }

private:
  auto Create(Span<const DescriptorSetLayoutBinding> bindings)
      -> UniquePtr<DescriptorSetLayoutResource> {

    Array<VkDescriptorSetLayoutBinding> vkBindings;
    vkBindings.Reserve(bindings.GetSize());

    for (const auto &binding : bindings) {
      vkBindings.PushBack({
          .binding = binding.binding,
          .descriptorType = ToVkDescriptorType(binding.type),
          .descriptorCount = binding.count,
          .stageFlags = ToVkShaderStageFlags(binding.visibleStages),
          .pImmutableSamplers = nullptr,
      });
    }

    VkDescriptorSetLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(vkBindings.GetSize()),
        .pBindings = vkBindings.GetData(),
    };

    VkDescriptorSetLayout setLayout{VK_NULL_HANDLE};
    auto vkResult =
        vkCreateDescriptorSetLayout(m_device, &info, nullptr, &setLayout);

    if (vkResult != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create descriptor set layout! Error code: {}",
            ToView(vkResult));
      return {};
    }

    return MakeUnique<DescriptorSetLayoutResource>(m_device, setLayout,
                                                   std::move(vkBindings));
  }

  VkDevice m_device;
  VkDescriptorSetLayout m_emptyLayout{VK_NULL_HANDLE};
  HashMap<HashType, UniquePtr<DescriptorSetLayoutResource>> m_cache;
};

} // namespace avalon::rhi
