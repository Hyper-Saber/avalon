module;
#include <cstdint>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:shader_module_cache;
import avalon.core;
import avalon.rhi;
import :utils;

namespace avalon::rhi {
class ShaderModuleCache : public NonCopyable,
                          public mem::AutoDestroyable<ShaderModuleCache> {
public:
  explicit ShaderModuleCache(VkDevice device) : m_device(device) {}

  ~ShaderModuleCache() {
    for (auto entry : m_cache) {
      auto module = entry.GetValue();
      vkDestroyShaderModule(m_device, module, nullptr);
    }
    m_cache.Clear();
  }

  VkShaderModule GetOrCreateShaderModule(const ShaderStageInfo &info) {
    auto hash = info.GetHash();
    if (m_cache.Contains(hash)) {
      return *m_cache.Get(hash);
    }

    VkShaderModule module{VK_NULL_HANDLE};

    VkShaderModuleCreateInfo moduleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = info.shaderCode->GetSize(),
        .pCode = info.shaderCode->ConstAs<uint32_t>(),
    };

    auto result =
        vkCreateShaderModule(m_device, &moduleCreateInfo, nullptr, &module);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create shader module! Error code: {}",
                    ToView(result));
      return module;
    }

    m_cache.Insert(hash, module);
    return module;
  }

private:
  VkDevice m_device;
  HashMap<HashType, VkShaderModule> m_cache;
};
} // namespace avalon::rhi
