module;
#include <debug/assert.hpp>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:resource_pool;

import avalon.core;

import :types;
import :utils;

namespace avalon::rhi {
class ResourcePool final : public NonCopyable,
                           public mem::AutoDestroyable<ResourcePool> {
public:
  explicit ResourcePool(IDeviceContext &context) : m_deviceContext(context) {}

  auto CreateBuffer(const BufferCreateInfo &info) -> Handle<BufferResource> {
    VkBufferCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<VkDeviceSize>(info.size),
        .usage = ToVkBufferUsageFlags(info.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    if (vkCreateBuffer(m_deviceContext.GetDevice(), &createInfo, nullptr,
                       &buffer) != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create buffer!");
      return {};
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_deviceContext.GetDevice(), buffer,
                                  &memRequirements);

    auto memoryTypeIndex =
        FindMemoryType(memRequirements.memoryTypeBits,
                       ToVkMemoryPropertyFlags(info.memoryProperty), info.size);

    if (memoryTypeIndex == kInvalidMemoryTypeIndex &&
        (info.memoryProperty & EMemoryProperty::DeviceLocal) !=
            EMemoryProperty::None) {
      auto fallback = info.memoryProperty & ~EMemoryProperty::DeviceLocal;
      if (fallback == EMemoryProperty::None) {
        fallback = EMemoryProperty::HostVisible | EMemoryProperty::HostCoherent;
      }
      memoryTypeIndex =
          FindMemoryType(memRequirements.memoryTypeBits,
                         ToVkMemoryPropertyFlags(fallback), info.size);

      Warn("[Vulkan] CRITICAL PERFORMANCE WARNING: Failed to allocate "
           "DeviceLocal memory "
           "for Buffer (Size: {:.2f} MB). The requested heap is either full or "
           "under high pressure. "
           "Falling back to System RAM (HostVisible). Expect increased PCIe "
           "bus overhead.",
           memRequirements.size / 1024.0 / 1024.0);
    }

    if (memoryTypeIndex == kInvalidMemoryTypeIndex) {
      vkDestroyBuffer(m_deviceContext.GetDevice(), buffer, nullptr);
      avalon::Error("[Vulkan] Failed to find suitable memory type for buffer!");
      return {};
    }

    AVALON_ASSERT(memoryTypeIndex != kInvalidMemoryTypeIndex);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };

    VkMemoryAllocateFlagsInfo flagsInfo{};
    if (HasFlag(info.usage, EResourceUsage::StorageBuffer)) {
      flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
      flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

      allocInfo.pNext = &flagsInfo;
    }

    VkDeviceMemory bufferMemory;
    if (vkAllocateMemory(m_deviceContext.GetDevice(), &allocInfo, nullptr,
                         &bufferMemory) != VK_SUCCESS) {
      vkDestroyBuffer(m_deviceContext.GetDevice(), buffer, nullptr);
      avalon::Error("Vulkan: Failed to allocate buffer memory!");
      return {};
    }

    if (vkBindBufferMemory(m_deviceContext.GetDevice(), buffer, bufferMemory,
                           0) != VK_SUCCESS) {
      vkDestroyBuffer(m_deviceContext.GetDevice(), buffer, nullptr);
      vkFreeMemory(m_deviceContext.GetDevice(), bufferMemory, nullptr);
      avalon::Error("[Vulkan]: Failed to bind buffer memory!");
      return {};
    }

    auto handle = m_bufferPool.Create(m_deviceContext.GetDevice(), buffer,
                                      bufferMemory, info.size);
    return handle;
  }

  auto ImportExternalTexture(VkDevice device, VkImage image, VkImageView view,
                             TextureCreateInfo info, bool isSwapchainTexture)
      -> Handle<TextureResource> {
    return m_texturePool.Create(m_deviceContext.GetDevice(), image, view,
                                VK_NULL_HANDLE, info, isSwapchainTexture);
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
        .arrayLayers = info.layerCount,
        .samples = ToVkSampleCount(info.sampleCount),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = ToVkImageUsageFlags(info.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (info.textureType == ETextureType::TextureCube) {
      imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VkImage image{VK_NULL_HANDLE};

    auto result =
        vkCreateImage(m_deviceContext.GetDevice(), &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create image! Error code: {}.",
            ToView(result));
      return {};
    }

    VkMemoryRequirements requirement;
    vkGetImageMemoryRequirements(m_deviceContext.GetDevice(), image,
                                 &requirement);
    auto memoryTypeIndex = FindMemoryType(requirement.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          requirement.size, false);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirement.size,
        .memoryTypeIndex = memoryTypeIndex,
    };

    VkDeviceMemory pMemory;
    result = vkAllocateMemory(m_deviceContext.GetDevice(), &allocInfo, nullptr,
                              &pMemory);

    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to allocate image memory! Error code: {}.",
            ToView(result));
      vkDestroyImage(m_deviceContext.GetDevice(), image, nullptr);
      return {};
    }

    result = vkBindImageMemory(m_deviceContext.GetDevice(), image, pMemory, 0);

    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to bind image memory! Error code: {}.",
            ToView(result));
      vkDestroyImage(m_deviceContext.GetDevice(), image, nullptr);
      vkFreeMemory(m_deviceContext.GetDevice(), pMemory, nullptr);
      return {};
    }

    VkImageAspectFlags aspectMask =
        isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    aspectMask = hasStencilComponent ? aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT
                                     : aspectMask;

    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (info.textureType == ETextureType::TextureCube) {
      viewType = info.layerCount > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
                                     : VK_IMAGE_VIEW_TYPE_CUBE;
    } else if (info.layerCount > 1) {
      viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    } else if (info.depth > 1) {
      viewType = VK_IMAGE_VIEW_TYPE_3D;
    }

    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = viewType,
        .format = vkFormat,
        .subresourceRange =
            {
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = info.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = info.layerCount,
            },
    };

    VkImageView view;
    result = vkCreateImageView(m_deviceContext.GetDevice(), &viewInfo, nullptr,
                               &view);
    if (result != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create image view! Error code: {}.",
            ToView(result));
      vkDestroyImage(m_deviceContext.GetDevice(), image, nullptr);
      vkFreeMemory(m_deviceContext.GetDevice(), pMemory, nullptr);
      return {};
    }

    auto handle = m_texturePool.Create(m_deviceContext.GetDevice(), image, view,
                                       pMemory, info);
    return handle;
  }

  auto GetOrCreateMipStorageView(Handle<TextureResource> handle,
                                 uint32_t mipLevel) -> VkImageView {
    auto *res = m_texturePool.Resolve(handle);
    AVALON_ASSERT(res != nullptr && "Invalid texture handle");
    AVALON_ASSERT(mipLevel < res->createInfo.mipLevels &&
                  "[Vulkan]: Mip level out of range");

    TextureSubresourceKey key{
        .mipLevel = mipLevel,
        .arrayLayer = 0,
        .isArrayView = res->createInfo.textureType == ETextureType::TextureCube,
    };

    if (res->subresourceViews.Contains(key)) {
      return res->subresourceViews[key];
    }

    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (res->createInfo.layerCount > 1) {
      viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }

    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = res->image,
        .viewType = viewType,
        .format = ToVkFormat(res->createInfo.format),
        .subresourceRange =
            {
                .aspectMask = res->aspectMask,
                .baseMipLevel = mipLevel,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = res->createInfo.layerCount,
            },
    };

    VkImageView subView;
    VkResult result =
        vkCreateImageView(res->device, &viewInfo, nullptr, &subView);
    if (result != VK_SUCCESS) {
      avalon::Error(
          "[Vulkan]: Failed to create subresource storage view for Mip {}",
          mipLevel);
      return VK_NULL_HANDLE;
    }

    res->subresourceViews[key] = subView;
    return subView;
  }

  auto CreateSampler(const SamplerCreateInfo &info) -> Handle<SamplerResource> {
    auto hash = info.GetHash();
    if (m_samplerCache.Contains(hash)) {
      return *m_samplerCache.Get(hash);
    }

    VkSamplerCreateInfo vkInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = ToVkFilter(info.magFilter),
        .minFilter = ToVkFilter(info.minFilter),
        .mipmapMode = ToVkMipmapMode(info.mipmapMode),
        .addressModeU = ToVkAddressMode(info.addressModeU),
        .addressModeV = ToVkAddressMode(info.addressModeV),
        .addressModeW = ToVkAddressMode(info.addressModeW),
        .mipLodBias = info.mipLodBias,
        .anisotropyEnable = info.anisotropyEnable ? VK_TRUE : VK_FALSE,
        .maxAnisotropy =
            Min(m_deviceContext.GetCapabilities().limits.maxSamplerAnisotroy,
                info.maxAnisotropy),
        .compareEnable = info.compareEnable ? VK_TRUE : VK_FALSE,
        .compareOp = ToVkCompareOp(info.compareOp),
        .minLod = info.minLod,
        .maxLod = info.maxLod,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    VkSampler sampler;
    if (vkCreateSampler(m_deviceContext.GetDevice(), &vkInfo, nullptr,
                        &sampler) != VK_SUCCESS) {
      Error("[Vulkan]: Failed to create sampler!");
      return {};
    }

    auto handle = m_samplerPool.Create(m_deviceContext.GetDevice(), sampler);
    m_samplerCache.Insert(hash, handle);
    return handle;
  }

  auto ResolveBuffer(Handle<BufferResource> handle) {
    return m_bufferPool.Resolve(handle);
  }

  auto ResolveTexture(Handle<TextureResource> handle) {
    return m_texturePool.Resolve(handle);
  }

  auto ResolveSampler(Handle<SamplerResource> handle) {
    return m_samplerPool.Resolve(handle);
  }

  void ReleaseBuffer(Handle<BufferResource> handle) {
    m_bufferPool.Release(handle);
  }
  void ReleaseTexture(Handle<TextureResource> handle) {
    m_texturePool.Release(handle);
  }

private:
  constexpr static uint32_t kInvalidMemoryTypeIndex = 0xFFFFFFFF;

  uint32_t FindMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags propertyFlags,
                          VkDeviceSize size, bool isBuffer = true) {
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps{
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
    VkPhysicalDeviceMemoryProperties2 memProps2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
        .pNext = &budgetProps};

    vkGetPhysicalDeviceMemoryProperties2(m_deviceContext.GetPhysicalDevice(),
                                         &memProps2);

    for (uint32_t i = 0; i < memProps2.memoryProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProps2.memoryProperties.memoryTypes[i].propertyFlags &
           propertyFlags) == propertyFlags) {

        uint32_t heapIndex =
            memProps2.memoryProperties.memoryTypes[i].heapIndex;
        VkDeviceSize budget = budgetProps.heapBudget[heapIndex];
        VkDeviceSize usage = budgetProps.heapUsage[heapIndex];

        VkDeviceSize safeMargin = 0;
        if (isBuffer) {
          VkDeviceSize totalHeapSize =
              memProps2.memoryProperties.memoryHeaps[heapIndex].size;
          safeMargin = Clamp(totalHeapSize / 10,
                             static_cast<VkDeviceSize>(16 * 1024 * 1024),
                             static_cast<VkDeviceSize>(128 * 1024 * 1024));
        }

        auto available = budget > usage ? budget - usage : 0;
        if (size + safeMargin <= available) {
          return i;
        } else {
          Warn("[Vulkan]: Memory type {} on Heap {} has enough space ({:.2f} "
               "MB), "
               "but failed to meet the Safe Margin requirement ({:.2f} MB). "
               "Rejecting to prevent heap exhaustion.",
               i, heapIndex, available / 1024.0 / 1024.0,
               safeMargin / 1024.0 / 1024.0);
        }
      }
    }

    return kInvalidMemoryTypeIndex;
  }

private:
  mem::ResourcePool<BufferResource> m_bufferPool;
  mem::ResourcePool<TextureResource> m_texturePool;
  mem::ResourcePool<SamplerResource> m_samplerPool;

  HashMap<HashType, Handle<SamplerResource>> m_samplerCache;

  IDeviceContext &m_deviceContext;

  uint32_t m_currentSwapchainFrameBufferIndex = 0;
};
} // namespace avalon::rhi
