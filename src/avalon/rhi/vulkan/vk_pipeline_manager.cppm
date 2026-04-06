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

    auto pipeline = BuildGraphicsPipeline(info);
    if (pipeline.IsValid())
      m_pipelineCaches.Insert(key, pipeline);
    return {pipeline.id};
  }

  auto GetOrCreate(const ComputePipelineCreateInfo &info)
      -> Handle<PipelineResource> {
    PipelineKey key{};
    key.renderingHash = 0;
    key.statePacked = 0;
    key.shaderHash = info.stageInfo.GetHash();

    HashType layoutHash = Hash::kOffsetBasis;
    for (const auto &binding : info.descriptorSetLayoutBindings) {
      layoutHash = Hash::Combine(layoutHash, binding.GetHash());
    }
    key.layoutHash = layoutHash;

    if (auto handle = m_pipelineCaches.Get(key)) {
      return *handle;
    }

    auto pipeline = BuildComputePipeline(info);
    if (pipeline.IsValid())
      m_pipelineCaches.Insert(key, pipeline);
    return {pipeline.id};
  }

  auto Resolve(Handle<PipelineResource> handle) -> PipelineResource * {
    return m_pipelinePool.Resolve(handle);
  }

private:
  struct PipelineKey {
    HashType renderingHash;
    HashType shaderHash;
    HashType layoutHash;
    HashType statePacked;

    bool operator==(const PipelineKey &other) const {
      return renderingHash == other.renderingHash &&
             shaderHash == other.shaderHash && layoutHash == other.layoutHash &&
             statePacked == other.statePacked;
    }
  };

  struct PipelineKeyHasher {
    auto operator()(const PipelineKey &key) const -> HashType {
      HashType hash = Hash::kOffsetBasis;
      hash = Hash::Combine(hash, key.renderingHash);
      hash = Hash::Combine(hash, key.shaderHash);
      hash = Hash::Combine(hash, key.layoutHash);
      return Hash::Combine(hash, key.statePacked);
    }
  };

  void ResolveDescriptorLayouts(Span<const DescriptorSetLayoutBinding> bindings,
                                Array<VkDescriptorSetLayout> &outVkLayouts,
                                Array<DescriptorSetLayoutMeta> &outMeta) {

    auto bindlessLayout = m_resourceProvider.GetBindlessSetLayout();
    auto globalSetLayout = m_resourceProvider.GetSceneGlobalSetLayout();

    outVkLayouts.Clear();
    outVkLayouts.Resize(kInternalSetCount);
    outVkLayouts[kBindlessSet] = bindlessLayout;
    outVkLayouts[kSceneGlobalsSet] = globalSetLayout;

    if (bindings.IsEmpty())
      return;

    uint32_t startIdx = 0;
    uint32_t currentSet = bindings[0].set;
    uint32_t total = bindings.GetSize();

    for (uint32_t i = 0; i <= total; ++i) {
      bool isEnd = (i == total);
      if (isEnd || bindings[i].set != currentSet) {
        auto subSpan = bindings.Subspan(startIdx, i - startIdx);

        while (outVkLayouts.GetSize() < currentSet) {
          outVkLayouts.PushBack(m_descriptorSetLayoutCache->GetEmptyLayout());
        }

        Span<const VkDescriptorSetLayoutBinding> vkBindings;
        auto setLayout =
            m_descriptorSetLayoutCache->GetOrCreate(subSpan, vkBindings);

        outVkLayouts.PushBack(setLayout);
        outMeta.PushBack(
            DescriptorSetLayoutMeta(subSpan, vkBindings, setLayout));

        if (isEnd)
          break;
        currentSet = bindings[i].set;
        startIdx = i;
      }
    }
  }

  auto BuildGraphicsPipeline(const PipelineCreateInfo &info)
      -> Handle<PipelineResource> {
    auto builder = PipelineBuilder();
    builder.LoadStates(info).SetVertexInput(info.vertexBindings,
                                            info.vertexInputAttributes);

    for (const auto &stageInfo : info.stageInfos) {
      auto module = m_shaderModuleCache->GetOrCreateShaderModule(stageInfo);
      builder.AddShaderStage(stageInfo.stage, stageInfo.entryName, module);
    }

    Array<VkDescriptorSetLayout> vkSetLayouts;
    Array<DescriptorSetLayoutMeta> meta;
    ResolveDescriptorLayouts(info.descriptorSetLayoutBindings, vkSetLayouts,
                             meta);

    Array<VkPushConstantRange> vkRanges;
    vkRanges.PushBack({
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = sizeof(StandardPushConstant),
    });

    auto layout =
        m_pipelineLayoutCache->GetOrCreateLayout(vkSetLayouts, vkRanges);
    auto pipeline = builder.Build(m_device, layout);

    if (pipeline == VK_NULL_HANDLE)
      return {};

    return m_pipelinePool.Create(m_device, pipeline, layout, std::move(meta));
  }

  auto BuildComputePipeline(const ComputePipelineCreateInfo &info)
      -> Handle<PipelineResource> {
    VkShaderModule csModule =
        m_shaderModuleCache->GetOrCreateShaderModule(info.stageInfo);

    Array<VkDescriptorSetLayout> vkSetLayouts;
    Array<DescriptorSetLayoutMeta> meta;
    ResolveDescriptorLayouts(info.descriptorSetLayoutBindings, vkSetLayouts,
                             meta);

    Array<VkPushConstantRange> vkRanges;
    vkRanges.PushBack({
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(StandardPushConstant),
    });

    auto layout =
        m_pipelineLayoutCache->GetOrCreateLayout(vkSetLayouts, vkRanges);

    VkComputePipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = csModule,
                .pName = info.stageInfo.entryName.GetData(),
            },
        .layout = layout,
    };

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr,
                             &vkPipeline);

    if (vkPipeline == VK_NULL_HANDLE)
      return {};

    return m_pipelinePool.Create(m_device, vkPipeline, layout, std::move(meta));
  }

  auto CreateInfoToKey(const PipelineCreateInfo &info) -> PipelineKey {
    HashType renderingHash = Hash::kOffsetBasis;
    for (const auto &format : info.renderingInfo.colorAttachmentFormats) {
      renderingHash =
          Hash::Combine(renderingHash, static_cast<HashType>(format));
    }
    renderingHash = Hash::Combine(
        renderingHash,
        static_cast<HashType>(info.renderingInfo.depthAttachmentFormat));
    renderingHash = Hash::Combine(
        renderingHash,
        static_cast<HashType>(info.renderingInfo.stencilAttachmentFormat));
    renderingHash = Hash::Combine(renderingHash, info.renderingInfo.viewMask);

    HashType shaderHash = Hash::kOffsetBasis;
    for (const auto &stage : info.stageInfos) {
      shaderHash = Hash::Combine(shaderHash, stage.GetHash());
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

    HashType stateHash = Hash::kOffsetBasis;
    stateHash = Hash::Combine(stateHash, info.inputAssemblyState.GetHash());
    stateHash = Hash::Combine(stateHash, info.rasterizationState.GetHash());
    stateHash = Hash::Combine(stateHash, info.depthStencilState.GetHash());
    stateHash = Hash::Combine(stateHash, info.multisampleState.GetHash());

    for (const auto &blend : info.colorBlendStates) {
      stateHash = Hash::Combine(stateHash, blend.GetHash());
    }

    return PipelineKey{.renderingHash = renderingHash,
                       .shaderHash = shaderHash,
                       .layoutHash = layoutHash,
                       .statePacked = stateHash};
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
