module;
#include <debug/assert.hpp>
#include <deque>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:descriptor_writer;

import :descriptor_provider;
import :types;
import avalon.core;
import avalon.rhi;

namespace avalon::rhi {

class DescriptorWriter final : public NonCopyable,
                               public IDescriptorWriter,
                               public mem::AutoDestroyable<DescriptorWriter> {
public:
  DescriptorWriter(VkDevice device, IRenderResourceProvider &provider)
      : m_device(device), m_resourceProvider(provider) {
    m_isSceneGlobalSet = true;
  }

  DescriptorWriter(VkDevice device, IRenderResourceProvider &provider,
                   DescriptorProvider &allocator, PipelineHandle pipelineHandle,
                   uint32_t setIndex)
      : m_device(device), m_resourceProvider(provider), m_allocator(&allocator),
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

  bool IsValid() const noexcept override {
    return m_isValid || m_isSceneGlobalSet;
  }

  auto WriteBuffer(StringId id, const BufferWriteInfo &info)
      -> IDescriptorWriter & override {

    uint32_t bindingPoint;
    VkDescriptorType type;

    if (m_isSceneGlobalSet) {
      bindingPoint = 0;
      type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    } else {
      auto binding = m_meta->Get(id);
      if (!binding) {
        Error("[Vulkan]: Id [{}] not found in descriptor set layout!",
              id.Resolve());
        return *this;
      }
      bindingPoint = binding->binding;
      type = binding->descriptorType;
    }

    VkDescriptorBufferInfo bufferInfo{
        .buffer = m_resourceProvider.GetBuffer(info.buffer)->buffer,
        .offset = info.offset,
        .range = info.range,
    };

    m_bufferInfos.push_back(bufferInfo);
    m_writes.PushBack({.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                       .dstBinding = bindingPoint,
                       .descriptorCount = 1,
                       .descriptorType = type,
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
    VkDescriptorSet set;
    DescriptorSetHandle handle;
    if (m_isSceneGlobalSet) {
      handle = m_resourceProvider.GetSceneGlobalSetHandle();
      set = m_resourceProvider.GetSceneGlobalSet();
    } else {
      auto setLayout = m_meta->setLayout;
      auto allocation =
          m_allocator->Allocate(setLayout, EDescriptorLifetime::PerFrame);

      if (!allocation) {
        return {};
      }

      handle = {allocation->handle.id};
      set = allocation->set;
    }

    for (auto &write : m_writes) {
      Debug("descriptorType: {}",
            write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
      write.dstSet = set;
    }

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(m_writes.GetSize()),
                           m_writes.GetData(), 0, nullptr);
    return handle;
  }

private:
  VkDevice m_device;
  DescriptorProvider *m_allocator;
  IRenderResourceProvider &m_resourceProvider;
  const DescriptorSetLayoutMeta *m_meta;
  uint32_t m_setIndex;

  bool m_isValid;
  bool m_isSceneGlobalSet;

  Array<VkWriteDescriptorSet> m_writes;
  std::deque<VkDescriptorBufferInfo> m_bufferInfos;
  std::deque<VkDescriptorImageInfo> m_imageInfos;
};

} // namespace avalon::rhi
