module;
#include <algorithm>
#include <debug/assert.hpp>
#include <deque>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:descriptor_writer;

import :descriptor_allocator;
import :types;
import avalon.core;
import avalon.rhi;

namespace avalon::rhi {

class DescriptorWriter final : public NonCopyable,
                               public IDescriptorWriter,
                               public mem::AutoDestroyable<DescriptorWriter> {
public:
  DescriptorWriter(VkDevice device, IRenderResourceProvider &provider,
                   DescriptorAllocator &allocator,
                   PipelineHandle pipelineHandle, uint32_t setIndex)
      : m_device(device), m_resourceProvider(provider), m_allocator(allocator),
        m_setIndex(setIndex) {

    auto pipelineResource = m_resourceProvider.GetPipeline({pipelineHandle});
    auto &maps = pipelineResource->descSetLayoutMaps;
    if (maps.GetSize() != 0 && setIndex < maps.GetSize()) {
      m_meta = &maps[setIndex];
      m_isValid = true;
    } else {
      m_isValid = false;
    }
  }

  bool IsValid() const noexcept override { return m_isValid; }

  auto WriteBuffer(StringId id, const BufferWriteInfo &info)
      -> IDescriptorWriter & override {

    auto binding = m_meta->Get(id);
    if (!binding) {
      Error("[Vulkan]: Id [{}] not found in descriptor set layout!",
            id.Resolve());
      return *this;
    }

    VkDescriptorBufferInfo bufferInfo{
        .buffer = m_resourceProvider.GetBuffer(info.buffer)->buffer,
        .offset = info.offset,
        .range = info.range,
    };

    m_bufferInfos.push_back(bufferInfo);
    m_writes.PushBack({.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                       .dstBinding = binding->binding,
                       .descriptorCount = 1,
                       .descriptorType = binding->descriptorType,
                       .pBufferInfo = &m_bufferInfos.back(),
                       .pTexelBufferView = nullptr});
    return *this;
  }

  auto WriteTexture(StringId id, TextureHandle texture, SamplerHandle sampler)
      -> IDescriptorWriter & override {
    auto binding = m_meta->Get(id);
    if (!binding) {
      Error("[Vulkan]: Id [{}] not found in descriptor set layout!",
            id.Resolve());
      return *this;
    }

    auto textureRes = m_resourceProvider.GetTexture(texture);
    auto samplerRes = m_resourceProvider.GetSampler(sampler);
    VkDescriptorImageInfo info{
        .sampler = samplerRes->sampler,
        .imageView = textureRes->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    m_imageInfos.push_back(info);

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = binding->binding,
        .descriptorCount = 1,
        .descriptorType = binding->descriptorType,
        .pImageInfo = &m_imageInfos.back(),
    };
    m_writes.PushBack(write);

    return *this;
  }

  auto Build() -> DescriptorSetHandle override {
    auto handle = m_allocator.Allocate(m_meta->setLayout);

    if (!handle.IsValid()) {
      return {};
    }

    auto setRes = m_allocator.Resolve(handle);
    auto set = setRes->descriptorSet;

    for (auto &write : m_writes) {
      write.dstSet = set;
    }

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(m_writes.GetSize()),
                           m_writes.GetData(), 0, nullptr);
    return {handle.id};
  }

private:
  VkDevice m_device;
  DescriptorAllocator &m_allocator;
  IRenderResourceProvider &m_resourceProvider;
  const DescriptorSetLayoutMeta *m_meta;
  uint32_t m_setIndex;

  bool m_isValid;

  Array<VkWriteDescriptorSet> m_writes;
  std::deque<VkDescriptorBufferInfo> m_bufferInfos;
  std::deque<VkDescriptorImageInfo> m_imageInfos;
};

} // namespace avalon::rhi
