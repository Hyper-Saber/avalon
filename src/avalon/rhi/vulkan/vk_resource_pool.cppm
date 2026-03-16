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

    if (memoryTypeIndex == kInvalidMemoryTypeIndex &&
        (info.memoryProperty & EMemoryProperty::DeviceLocal) !=
            EMemoryProperty::None) {
      auto fallback = info.memoryProperty & ~EMemoryProperty::DeviceLocal;
      if (fallback != EMemoryProperty::None) {
        memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
                                         ToVkMemoryPropertyFlags(fallback));

        Warn("[Vulkan] Failed to allocate DeviceLocal memory for buffer. "
             "Falling back to System RAM (HostVisible).");
      }
    }

    if (memoryTypeIndex == kInvalidMemoryTypeIndex) {
      vkDestroyBuffer(m_device, buffer, nullptr);
      avalon::Error("[Vulkan] Failed to find suitable memory type for buffer!");
      return {};
    }

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
                                   VkImageView depthView, uint32_t width,
                                   uint32_t height, uint32_t layers)
      -> Array<Handle<FrameBufferResource>> {

    auto frameBuffers = Array<Handle<FrameBufferResource>>(views.GetSize());

    auto pass = m_renderPassCache->Resolve({handle.id})->renderPass;
    uint32_t i = 0;
    for (const auto &view : views) {
      auto totalViews = Array<VkImageView>{view};
      if (depthView != VK_NULL_HANDLE)
        totalViews.PushBack(depthView);
      frameBuffers[i++] =
          CreateFrameBuffer(pass, totalViews, width, height, layers);
    }
    return frameBuffers;
  }

  auto CreateTexture(const TextureCreateInfo &info) -> Handle<TextureResource> {
    auto vkFormat = ToVkFormat(info.format);
    bool isDepthFormat = IsDepthFormat(info.format);
    bool hasStencilComponent = HasStencilComponent(info.format);

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = (info.depth > 1) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D,
        .format = vkFormat,
        .extent =
            {
                info.width,
                info.height,
                info.depth,
            },
        .mipLevels = info.mipLevels,
        .arrayLayers = info.layers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (isDepthFormat) {
      imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    VkImage image{VK_NULL_HANDLE};

    auto result = vkCreateImage(m_device, &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create image! Error code: {}.",
            ToView(result));
      return {};
    }

    VkMemoryRequirements requirement;
    vkGetImageMemoryRequirements(m_device, image, &requirement);
    auto memoryTypeIndex = FindMemoryType(requirement.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirement.size,
        .memoryTypeIndex = memoryTypeIndex,
    };

    VkDeviceMemory pMemory;
    result = vkAllocateMemory(m_device, &allocInfo, nullptr, &pMemory);

    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to allocate image memory! Error code: {}.",
            ToView(result));
      vkDestroyImage(m_device, image, nullptr);
      return {};
    }

    result = vkBindImageMemory(m_device, image, pMemory, 0);

    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to bind image memory! Error code: {}.",
            ToView(result));
      vkDestroyImage(m_device, image, nullptr);
      vkFreeMemory(m_device, pMemory, nullptr);
      return {};
    }

    VkImageAspectFlags aspectMask =
        isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    aspectMask = hasStencilComponent ? aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT
                                     : aspectMask;

    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = info.layers > 1  ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                    : info.depth > 1 ? VK_IMAGE_VIEW_TYPE_3D
                                     : VK_IMAGE_VIEW_TYPE_2D,
        .format = vkFormat,
        .subresourceRange =
            {
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = info.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = info.layers,
            },
    };

    VkImageView view;
    result = vkCreateImageView(m_device, &viewInfo, nullptr, &view);
    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create image view! Error code: {}.",
            ToView(result));
      vkDestroyImage(m_device, image, nullptr);
      vkFreeMemory(m_device, pMemory, nullptr);
      return {};
    }

    return m_texturePool.Create(m_device, image, view, pMemory);
  } // namespace avalon::rhi

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

  auto ResolveTexture(Handle<TextureResource> handle) {
    return m_texturePool.Resolve(handle);
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
