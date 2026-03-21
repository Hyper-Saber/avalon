module;
#include <debug/assert.hpp>
#include <utility>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:pipeline_manager;

import avalon.core;
import :types;
import avalon.rhi;
import :pipeline_builder;
import :shader_module_cache;
import :pipeline_layout_cache;
import :descriptor_set_layout_cache;
import :utils;

namespace avalon::rhi {

class PipelineManager final : public NonCopyable,
                              public mem::AutoDestroyable<PipelineManager> {
public:
  PipelineManager(VkDevice device, IRenderResourceProvider &provider)
      : m_device(device), m_resourceProvider(provider) {
    m_shaderModuleCache = MakeUnique<ShaderModuleCache>(device);
    m_descriptorSetLayoutCache = MakeUnique<DescriptorSetLayoutCache>(device);
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

    HashType stateHash = Hash::kOffsetBasis;
    stateHash = Hash::Combine(stateHash, info.inputAssemblyState.GetHash());
    stateHash = Hash::Combine(stateHash, info.rasterizationState.GetHash());
    stateHash = Hash::Combine(stateHash, info.depthStencilState.GetHash());
    stateHash = Hash::Combine(stateHash, info.multisampleState.GetHash());

    for (const auto &blend : info.colorBlendStates) {
      stateHash = Hash::Combine(stateHash, blend.GetHash());
    }

    return PipelineKey{.renderPassHash =
                           static_cast<HashType>(info.renderPassHandle.id),
                       .shaderHash = shaderHash,
                       .layoutHash = layoutHash,
                       .statePacked = stateHash};
  }

  auto BuildPipeline(const PipelineCreateInfo &info)
      -> Handle<PipelineResource> {
    auto renderPass =
        m_resourceProvider.GetRenderPass(info.renderPassHandle)->renderPass;

    auto builder = PipelineBuilder();
    builder.SetRenderPass(renderPass)
        .LoadStates(info)
        .SetVertexInput(info.vertexBindings, info.vertexInputAttributes);

    for (const auto &stageInfo : info.stageInfos) {
      auto module = m_shaderModuleCache->GetOrCreateShaderModule(stageInfo);
      builder.AddShaderStage(stageInfo.stage, stageInfo.entryName, module);
    }

    Array<VkDescriptorSetLayout> vkSetLayouts;
    Array<DescriptorSetLayoutMeta> meta;
    uint32_t startIdx = 0;
    uint32_t set = info.descriptorSetLayoutBindings.IsEmpty()
                       ? 0
                       : info.descriptorSetLayoutBindings[0].set;
    auto total = info.descriptorSetLayoutBindings.GetSize();
    for (uint32_t i = 0; i <= total; ++i) {
      bool isEnd = (i == total);
      if (isEnd || info.descriptorSetLayoutBindings[i].set != set) {
        auto subSpan =
            info.descriptorSetLayoutBindings.Subspan(startIdx, i - startIdx);
        if (!subSpan.IsEmpty()) {
          while (vkSetLayouts.GetSize() < set) {
            vkSetLayouts.PushBack(VK_NULL_HANDLE);
          }

          Span<const VkDescriptorSetLayoutBinding> vkBindings;
          auto setLayout =
              m_descriptorSetLayoutCache->GetOrCreate(subSpan, vkBindings);
          if (setLayout == VK_NULL_HANDLE)
            return {};
          vkSetLayouts.PushBack(setLayout);
          meta.PushBack(
              DescriptorSetLayoutMeta(subSpan, vkBindings, setLayout));
        }
        if (isEnd) {
          break;
        }

        AVALON_ASSERT(info.descriptorSetLayoutBindings[i].set > set);

        set = info.descriptorSetLayoutBindings[i].set;
        startIdx = i;
      }
    }

    uint32_t i = 0;
    Array<VkPushConstantRange> ranges(info.pushConstantRanges.GetSize());
    for (const auto &range : info.pushConstantRanges) {
      ranges[i] = {
          .stageFlags = ToVkShaderStageFlags(range.visibleStages),
          .offset = range.offset,
          .size = range.size,
      };
    }

    VkPushConstantRange range;
    auto layout =
        m_pipelineLayoutCache->GetOrCreateLayout(vkSetLayouts, ranges);
    auto pipeline = builder.Build(m_device, layout);
    if (pipeline == VK_NULL_HANDLE)
      return {};

    return m_pipelinePool.Create(m_device, pipeline, layout, std::move(meta));
  }

  VkDevice m_device;
  IRenderResourceProvider &m_resourceProvider;
  mem::ResourcePool<PipelineResource> m_pipelinePool;
  HashMap<PipelineKey, Handle<PipelineResource>, PipelineKeyHasher>
      m_pipelineCaches;
  UniquePtr<ShaderModuleCache> m_shaderModuleCache;
  UniquePtr<DescriptorSetLayoutCache> m_descriptorSetLayoutCache;
  UniquePtr<PipelineLayoutCache> m_pipelineLayoutCache;
};
} // namespace avalon::rhi
