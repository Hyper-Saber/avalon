module;
#include <debug/assert.hpp>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:resource_pool;

import avalon.core;

import :types;
import :render_pass_cache;

namespace avalon::rhi {
class ResourcePool final : public NonCopyable,
                           public mem::AutoDestroyable<ResourcePool> {
public:
  explicit ResourcePool(VkDevice device, VkPhysicalDevice physicalDeveice)
      : m_device(device), m_physicalDevice(physicalDeveice) {
    m_renderPassCache = MakeUnique<RenderPassCache>(device);
  }

  auto CreateBuffer(const BufferCreateInfo &info) -> Handle<BufferResource> {
    VkBufferCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<VkDeviceSize>(info.size),
        .usage = ToVkBufferUsageFlags(info.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    if (vkCreateBuffer(m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create buffer!");
      return {};
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    auto memoryTypeIndex =
        FindMemoryType(memRequirements.memoryTypeBits,
                       ToVkMemoryPropertyFlags(info.memoryProperty));

    AVALON_ASSERT(memoryTypeIndex != kInvalidMemoryTypeIndex);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };

    VkDeviceMemory bufferMemory;
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) !=
        VK_SUCCESS) {
      vkDestroyBuffer(m_device, buffer, nullptr);
      avalon::Error("Vulkan: Failed to allocate buffer memory!");
      return {};
    }

    if (vkBindBufferMemory(m_device, buffer, bufferMemory, 0) != VK_SUCCESS) {
      vkDestroyBuffer(m_device, buffer, nullptr);
      vkFreeMemory(m_device, bufferMemory, nullptr);
      avalon::Error("[Vulkan]: Failed to bind buffer memory!");
      return {};
    }

    auto handle =
        m_bufferPool.Create(m_device, buffer, bufferMemory, info.size);
    return handle;
  }

  auto CreateFrameBuffer(const FrameBufferCreateInfo &createInfo)
      -> Handle<FrameBufferResource> {
    Array<VkImageView> views(createInfo.attachments.GetSize());

    for (uint32_t i = 0; i < views.GetSize(); i++) {
      views[i] = m_texturePool
                     .Resolve(Handle<TextureResource>{
                         .id = createInfo.attachments[i].id})
                     ->imageView;
    }

    auto pass = m_renderPassCache->Resolve({createInfo.renderPassHandle.id})
                    ->renderPass;

    return CreateFrameBuffer(pass, views, createInfo.width, createInfo.height,
                             createInfo.layers);
  }

  auto CreateSwapchainFrameBuffers(RenderPassHandle handle,
                                   const Array<VkImageView> &views,
                                   uint32_t width, uint32_t height,
                                   uint32_t layers)
      -> Array<Handle<FrameBufferResource>> {

    auto frameBuffers = Array<Handle<FrameBufferResource>>(views.GetSize());

    auto pass = m_renderPassCache->Resolve({handle.id})->renderPass;
    uint32_t i = 0;
    for (const auto &view : views) {
      frameBuffers[i++] =
          CreateFrameBuffer(pass, {view}, width, height, layers);
    }
    return frameBuffers;
  }

  auto CreateTexture(const TextureCreateInfo &info) -> Handle<TextureResource> {
  }

  auto CreateRenderPass(const RenderPassCreateInfo &info)
      -> Handle<RenderPassResource> {
    return m_renderPassCache->GetOrCreateRenderPass(info);
  }

  auto ResolveRenderPass(Handle<RenderPassResource> handle) {
    return m_renderPassCache->Resolve(handle);
  }

  auto ResolveBuffer(Handle<BufferResource> handle) {
    return m_bufferPool.Resolve(handle);
  }

  auto ResolveFrameBuffer(Handle<FrameBufferResource> handle) {
    return m_frameBufferPool.Resolve(handle);
  }

  void ReleaseBuffer(Handle<BufferResource> handle) {
    m_bufferPool.Destroy(handle);
  }
  void ReleaseTexture(Handle<TextureResource> handle) {
    m_texturePool.Destroy(handle);
  }

  void ReleaseFrameBuffer(Handle<FrameBufferResource> handle) {
    m_frameBufferPool.Destroy(handle);
  }

private:
  constexpr static uint32_t kInvalidMemoryTypeIndex = 0xFFFFFFFF;

  auto CreateFrameBuffer(VkRenderPass pass, const Array<VkImageView> &views,
                         uint32_t width, uint32_t height, uint32_t layers)
      -> Handle<FrameBufferResource> {

    VkFramebuffer frameBuffer;
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pass,
        .attachmentCount = static_cast<uint32_t>(views.GetSize()),
        .pAttachments = views.GetData(),
        .width = width,
        .height = height,
        .layers = layers,
    };

    auto result =
        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &frameBuffer);
    if (result != VK_SUCCESS) {
      HandleVkError(result);
      return {};
    }

    auto frameHandle = m_frameBufferPool.Create(m_device, frameBuffer);
    return frameHandle;
  }

  uint32_t FindMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags propertyFlags) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & propertyFlags) ==
              propertyFlags) {
        return i;
      }
    }
    avalon::Error("Vulkan: Failed to find suitable memory type!");
    return kInvalidMemoryTypeIndex;
  }

private:
  mem::ResourcePool<BufferResource> m_bufferPool;
  mem::ResourcePool<TextureResource> m_texturePool;
  mem::ResourcePool<FrameBufferResource> m_frameBufferPool;
  UniquePtr<RenderPassCache> m_renderPassCache;

  VkDevice m_device;
  VkPhysicalDevice m_physicalDevice;

  uint32_t m_currentSwapchainFrameBufferIndex = 0;
};
} // namespace avalon::rhi
