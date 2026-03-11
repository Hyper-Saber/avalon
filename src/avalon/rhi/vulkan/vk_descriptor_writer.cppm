module;
#include <deque>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:descriptor_writer;

import :descriptor_allocator;
import avalon.core;

namespace avalon::rhi {

class DescriptorWriter : public NonCopyable {
public:
  DescriptorWriter(DescriptorAllocator &allocator, VkDescriptorSetLayout layout)
      : m_allocator(allocator), m_layout(layout) {}

  DescriptorWriter &WriteBuffer(uint32_t binding, VkBuffer buffer,
                                VkDescriptorType type, VkDeviceSize offset = 0,
                                VkDeviceSize range = VK_WHOLE_SIZE) {
    VkDescriptorBufferInfo bufferInfo{
        .buffer = buffer, .offset = offset, .range = range};

    m_bufferInfos.push_back(bufferInfo);
    m_writes.PushBack({.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                       .dstBinding = binding,
                       .descriptorCount = 1,
                       .descriptorType = type,
                       .pBufferInfo = &m_bufferInfos.back(),
                       .pTexelBufferView = nullptr});
    return *this;
  }

  DescriptorWriter &WriteImage(uint32_t binding, VkImageView imageView,
                               VkDescriptorType type, VkSampler sampler) {
    VkDescriptorImageInfo info{
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    m_imageInfos.push_back(info);

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &m_imageInfos.back(),
    };
    m_writes.PushBack(write);
    return *this;
  }

  auto Build(VkDevice device) -> VkDescriptorSet {
    auto set = m_allocator.Allocate(m_layout);
    if (set == VK_NULL_HANDLE) {
      return VK_NULL_HANDLE;
    }

    for (auto &write : m_writes) {
      write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_writes.GetSize()),
                           m_writes.GetData(), 0, nullptr);
    return set;
  }

private:
  DescriptorAllocator &m_allocator;
  VkDescriptorSetLayout m_layout;

  Array<VkWriteDescriptorSet> m_writes;
  std::deque<VkDescriptorBufferInfo> m_bufferInfos;
  std::deque<VkDescriptorImageInfo> m_imageInfos;
};

} // namespace avalon::rhi
