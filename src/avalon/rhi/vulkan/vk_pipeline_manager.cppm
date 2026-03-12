module;
#include <algorithm>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:pipeline_manager;

import avalon.core;
import :types;
import avalon.rhi;
import :pipeline_builder;
import :shader_module_cache;
import :pipeline_layout_cache;
import :utils;

namespace avalon::rhi {

class PipelineManager final : public NonCopyable,
                              public mem::AutoDestroyable<PipelineManager> {
public:
  PipelineManager(VkDevice device, IRenderResourceProvider &provider)
      : m_device(device), m_resourceProvider(provider) {
    m_shaderModuleCache = MakeUnique<ShaderModuleCache>(device);
    m_pipelineLayoutCache = MakeUnique<PipelineLayoutCache>(device);
  }

  auto GetOrCreate(const PipelineCreateInfo &info) -> Handle<PipelineResource> {
    auto key = CreateInfoToKey(info);
    if (auto handle = m_pipelineCaches.Get(key)) {
      return *handle;
    }

    auto pipeline = BuildPipeline(info);
    if (pipeline.IsValid())
      m_pipelineCaches.Insert(key, pipeline);
    return {pipeline.id};
  }

  auto Resolve(Handle<PipelineResource> handle) -> PipelineResource * {
    return m_pipelinePool.Resolve(handle);
  }

private:
  struct PipelineKey {
    HashType renderPassHash;
    HashType shaderHash;
    HashType layoutHash;
    HashType statePacked;

    bool operator==(const PipelineKey &other) const {
      return renderPassHash == other.renderPassHash &&
             shaderHash == other.shaderHash && layoutHash == other.layoutHash &&
             statePacked == other.statePacked;
    }
  };

  struct PipelineKeyHasher {
    auto operator()(const PipelineKey &key) const -> HashType {
      HashType hash = Hash::kOffsetBasis;
      hash = Hash::Combine(hash, key.renderPassHash);
      hash = Hash::Combine(hash, key.shaderHash);
      hash = Hash::Combine(hash, key.layoutHash);
      return Hash::Combine(hash, key.statePacked);
    }
  };

  auto CreateInfoToKey(const PipelineCreateInfo &info) -> PipelineKey {
    HashType shaderHash = Hash::kOffsetBasis;
    for (const auto &stages : info.stageInfos) {
      shaderHash = Hash::Combine(shaderHash, stages.GetHash());
    }
    HashType layoutHash = Hash::kOffsetBasis;
    for (const auto &binding : info.vertexBindings) {
      layoutHash = Hash::Combine(layoutHash, binding.GetHash());
    }
    for (const auto &attribute : info.vertexInputAttributes) {
      layoutHash = Hash::Combine(layoutHash, attribute.GetHash());
    }
    for (const auto &setLayoutBinding : info.descriptorSetLayoutBindings) {
      layoutHash = Hash::Combine(layoutHash, setLayoutBinding.GetHash());
    }
    for (const auto &range : info.pushConstantRanges) {
      layoutHash = Hash::Combine(layoutHash, range.GetHash());
    }

    HashType state = 0;
    state |= (static_cast<HashType>(info.topology) & 0xF);
    state |= (static_cast<HashType>(info.polygonMode) & 0xF) << 4;
    state |= (static_cast<HashType>(info.cullMode) & 0xF) << 8;
    state |= (static_cast<HashType>(info.isDepthTestEnable ? 1ULL : 0ULL) & 0x1)
             << 12;
    state |=
        (static_cast<HashType>(info.isDepthWriteEnable ? 1ULL : 0ULL) & 0x1)
        << 13;
    state |= (static_cast<HashType>(info.depthCompareOp) & 0xF) << 14;

    return PipelineKey{.renderPassHash =
                           static_cast<HashType>(info.renderPassHandle.id),
                       .shaderHash = shaderHash,
                       .layoutHash = layoutHash,
                       .statePacked = state};
  }

  auto BuildPipeline(const PipelineCreateInfo &info)
      -> Handle<PipelineResource> {
    auto renderPass =
        m_resourceProvider.GetRenderPass(info.renderPassHandle).renderPass;

    auto builder = PipelineBuilder();
    builder.SetRenderPass(renderPass)
        .LoadStates(info)
        .SetVertexInput(info.vertexBindings, info.vertexInputAttributes);

    for (const auto &stageInfo : info.stageInfos) {
      auto module = m_shaderModuleCache->GetOrCreateShaderModule(stageInfo);
      builder.AddShaderStage(stageInfo.stage, stageInfo.entryName, module);
    }

    uint32_t maxSet = 0;
    for (const auto &binding : info.descriptorSetLayoutBindings) {
      maxSet = std::max(maxSet, binding.set);
    }

    Array<Array<VkDescriptorSetLayoutBinding>> setGroups(maxSet + 1);
    for (const auto &binding : info.descriptorSetLayoutBindings) {
      setGroups[binding.set].PushBack({
          .binding = binding.binding,
          .descriptorType = ToVkDescriptorType(binding.type),
          .descriptorCount = binding.count,
          .stageFlags = ToVkShaderStageFlags(binding.visibleStages),
      });
    }

    Array<VkDescriptorSetLayout> setLayouts(setGroups.GetSize());
    uint32_t i = 0;
    for (const auto &group : setGroups) {
      VkDescriptorSetLayoutCreateInfo info{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
          .bindingCount = static_cast<uint32_t>(group.GetSize()),
          .pBindings = group.GetData(),
      };
      VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
      auto vkResult =
          vkCreateDescriptorSetLayout(m_device, &info, nullptr, &setLayout);
      if (vkResult != VK_SUCCESS) {
        Error(
            "[Vulkan]: Failed to create descriptor set layout! Error code: {}",
            ToView(vkResult));
        return {};
      }

      setLayouts[i++] = setLayout;
    }

    i = 0;
    Array<VkPushConstantRange> ranges(info.pushConstantRanges.GetSize());
    for (const auto &range : info.pushConstantRanges) {
      ranges[i] = {
          .stageFlags = ToVkShaderStageFlags(range.visibleStages),
          .offset = range.offset,
          .size = range.size,
      };
    }

    VkPushConstantRange range;
    auto layout = m_pipelineLayoutCache->GetOrCreateLayout(setLayouts, ranges);
    auto pipeline = builder.Build(m_device, layout);
    if (pipeline == VK_NULL_HANDLE)
      return {};

    return m_pipelinePool.Create(m_device, pipeline, layout,
                                 std::move(setLayouts));
  }

  VkDevice m_device;
  IRenderResourceProvider &m_resourceProvider;
  mem::ResourcePool<PipelineResource> m_pipelinePool;
  HashMap<PipelineKey, Handle<PipelineResource>, PipelineKeyHasher>
      m_pipelineCaches;
  UniquePtr<ShaderModuleCache> m_shaderModuleCache;
  UniquePtr<PipelineLayoutCache> m_pipelineLayoutCache;
};
} // namespace avalon::rhi
