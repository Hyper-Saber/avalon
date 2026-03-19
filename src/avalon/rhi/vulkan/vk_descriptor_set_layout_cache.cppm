module;
#include <algorithm>
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

  ~DescriptorSetLayoutCache() { m_cache.Clear(); }

  auto GetOrCreate(Span<const DescriptorSetLayoutBinding> &bindings,
                   Span<const VkDescriptorSetLayoutBinding> &outBindings)
      -> VkDescriptorSetLayout {
    if (bindings.IsEmpty())
      return VK_NULL_HANDLE;

    HashType hash = Hash::kOffsetBasis;
    for (const auto &b : bindings) {
      hash = Hash::Combine(hash, b.GetHash());
    }

    if (m_cache.Contains(hash)) {
      auto res = (*m_cache.Get(hash)).Get();
      outBindings = res->GetBindings();
      return res->setLayout;
    }

    auto layout = Create(bindings);

    if (layout.Get()) {
      outBindings = layout->GetBindings();
      auto ret = layout->setLayout;
      m_cache.Insert(hash, std::move(layout));
      return ret;
    }

    return VK_NULL_HANDLE;
  }

private:
  auto Create(Span<const DescriptorSetLayoutBinding> &bindings)
      -> UniquePtr<DescriptorSetLayoutResource> {
    Array<VkDescriptorSetLayoutBinding> vkBindings;

    Debug("[Vulkan]: DescriptorSetLayout: set {}", bindings[0].set);

    for (const auto &binding : bindings) {
      vkBindings.PushBack({
          .binding = binding.binding,
          .descriptorType = ToVkDescriptorType(binding.type),
          .descriptorCount = binding.count,
          .stageFlags = ToVkShaderStageFlags(binding.visibleStages),
      });
      Debug("\nbinding {}: "
            "\n---------------------------------------\ntype: {}\ncount: "
            "{}\nvisibleStages: {}"
            "\n---------------------------------------",
            binding.binding, ToView(binding.type), binding.count,
            ToView(binding.visibleStages));
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

    auto ret = MakeUnique<DescriptorSetLayoutResource>(m_device, setLayout,
                                                       std::move(vkBindings));

    return ret;
  }

  VkDevice m_device;
  HashMap<HashType, UniquePtr<DescriptorSetLayoutResource>> m_cache;
};
} // namespace avalon::rhi
