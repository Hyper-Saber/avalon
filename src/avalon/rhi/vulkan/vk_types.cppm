module;
#include <debug/assert.hpp>
#include <optional>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:types;
import avalon.core;
import avalon.rhi;

namespace avalon::rhi {

struct DeviceConfig {
  QueueRequirement queueRequirement;
  Array<const char *> extensions;
  VkPhysicalDeviceFeatures features;
};

struct DescriptorSetLayoutBindingMap {
  const StringId nameHash;
  const uint32_t index;
};

struct DescriptorSetLayoutMeta {
  const Span<const VkDescriptorSetLayoutBinding> bindings;
  const VkDescriptorSetLayout setLayout;
  Array<DescriptorSetLayoutBindingMap> maps;

  DescriptorSetLayoutMeta(Span<const DescriptorSetLayoutBinding> bindings,
                          Span<const VkDescriptorSetLayoutBinding> vkBindings,
                          VkDescriptorSetLayout setLayout)
      : bindings(vkBindings), setLayout(setLayout) {
    AVALON_ASSERT(bindings.GetSize() == vkBindings.GetSize());
    uint32_t lastBinding = 0;
    maps.Reserve(bindings.GetSize());
    for (uint32_t i = 0; i < bindings.GetSize(); ++i) {
      AVALON_ASSERT(bindings[i].binding == vkBindings[i].binding &&
                    bindings[i].binding >= lastBinding)
      maps.PushBack({
          .nameHash = bindings[i].nameHash,
          .index = i,
      });
      lastBinding = bindings[i].binding;
    }
  }

  auto Get(StringId nameHash) const -> const VkDescriptorSetLayoutBinding * {
    for (auto &map : maps) {
      if (map.nameHash == nameHash) {
        return &bindings[map.index];
      }
    }
    return nullptr;
  }
};

struct PipelineResource : public mem::AutoDestroyable<PipelineResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkPipeline pipeline{VK_NULL_HANDLE};
  const VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  const Array<DescriptorSetLayoutMeta> descSetLayoutMaps;
  EPipelineBindPoint bindPoint;

  PipelineResource(VkDevice device, VkPipeline pipeline,
                   VkPipelineLayout layout,
                   Array<DescriptorSetLayoutMeta> &&meta,
                   EPipelineBindPoint bindPoint = EPipelineBindPoint::Graphics)
      : device(device), pipeline(pipeline), pipelineLayout(layout),
        descSetLayoutMaps(std::move(meta)), bindPoint(bindPoint) {}

  ~PipelineResource() { vkDestroyPipeline(device, pipeline, nullptr); }
};

struct DescriptorSetLayoutResource
    : public mem::AutoDestroyable<DescriptorSetLayoutResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkDescriptorSetLayout setLayout{VK_NULL_HANDLE};
  const Array<VkDescriptorSetLayoutBinding> bindings;

  DescriptorSetLayoutResource(VkDevice device, VkDescriptorSetLayout setLayout,
                              Array<VkDescriptorSetLayoutBinding> &&bindings)
      : device(device), setLayout(setLayout), bindings(std::move(bindings)) {}

  ~DescriptorSetLayoutResource() {
    vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
  }

  auto GetBindings() -> Span<const VkDescriptorSetLayoutBinding> {
    return Span{bindings.GetData(), bindings.GetSize()};
  }
};

struct BufferResource : public mem::AutoDestroyable<BufferResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkBuffer buffer{VK_NULL_HANDLE};
  const VkDeviceMemory memory{VK_NULL_HANDLE};
  const size_t size = 0;

  BufferResource(VkDevice device, VkBuffer buffer, VkDeviceMemory memory,
                 size_t size)
      : device(device), buffer(buffer), memory(memory), size(size) {}

  ~BufferResource() {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
  }
};

struct TextureSubresourceKey {
  uint32_t mipLevel;
  uint32_t arrayLayer;
  bool isArrayView;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;

    packed |= (static_cast<uint64_t>(mipLevel) & 0xFFFFULL);

    packed |= (static_cast<uint64_t>(arrayLayer) & 0xFFFFULL) << 16;

    packed |= (static_cast<uint64_t>(isArrayView ? 1ULL : 0ULL)) << 32;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const TextureSubresourceKey &other) const noexcept {
    return mipLevel == other.mipLevel && arrayLayer == other.arrayLayer &&
           isArrayView == other.isArrayView;
  }
};

struct TextureResource : public mem::AutoDestroyable<TextureResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkImage image{VK_NULL_HANDLE};
  const VkImageView imageView{VK_NULL_HANDLE};
  const VkDeviceMemory memory{VK_NULL_HANDLE};
  VkImageAspectFlags aspectMask;
  const TextureCreateInfo createInfo;
  const bool isSwapchainTexture = false;

  HashMap<TextureSubresourceKey, VkImageView> subresourceViews;

  TextureResource(VkDevice device, VkImage image, VkImageView imageView,
                  VkDeviceMemory memory, const TextureCreateInfo &info,
                  bool isSwapChainTexture = false)
      : device(device), image(image), imageView(imageView), memory(memory),
        createInfo(info), isSwapchainTexture(isSwapChainTexture) {
    if (info.format == EFormat::R8G8B8_UNORM ||
        info.format == EFormat::R8G8B8A8_UNORM ||
        info.format == EFormat::R8G8B8_SRGB ||
        info.format == EFormat::B8G8R8A8_SRGB ||
        info.format == EFormat::R8G8B8A8_SRGB ||
        info.format == EFormat::R16_SFLOAT ||
        info.format == EFormat::R16G16_SFLOAT ||
        info.format == EFormat::R16G16B16A16_SFLOAT) {
      aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    } else if (info.format == EFormat::D32_SFLOAT) {
      aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (info.format == EFormat::D32_SFLOAT_S8_UINT) {
      aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }

  ~TextureResource() {
    for (auto &entry : subresourceViews) {
      vkDestroyImageView(device, entry.GetValue(), nullptr);
    }
    subresourceViews.Clear();

    vkDestroyImageView(device, imageView, nullptr);
    if (isSwapchainTexture)
      return;
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);
  }
};

struct SamplerResource : public mem::AutoDestroyable<SamplerResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkSampler sampler{VK_NULL_HANDLE};

  SamplerResource(VkDevice device, VkSampler sampler)
      : device(device), sampler(sampler) {}

  ~SamplerResource() { vkDestroySampler(device, sampler, nullptr); }
};

struct DescriptorSetResource
    : public mem::AutoDestroyable<DescriptorSetResource> {
  const VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
  const VkDescriptorSetLayout layout{VK_NULL_HANDLE};

  DescriptorSetResource(VkDescriptorSet descriptorSet,
                        VkDescriptorSetLayout layout)
      : descriptorSet(descriptorSet), layout(layout) {}
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  std::optional<uint32_t> computeFamily;
  std::optional<uint32_t> transferFamily;

  bool IsComplete(QueueRequirement requirement) {
    return requirement.isRequirePresent ? presentFamily.has_value()
           : !presentFamily.has_value() && requirement.isRequireCompute
               ? computeFamily.has_value()
           : !computeFamily.has_value() && requirement.isRequireTransfer
               ? transferFamily.has_value()
           : !transferFamily.has_value() && requirement.isRequireGraphics
               ? graphicsFamily.has_value()
               : !graphicsFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  avalon::Array<VkSurfaceFormatKHR> surfaceFormats;
  avalon::Array<VkPresentModeKHR> presentModes;
};

class IDeviceContext {
public:
  virtual auto GetInstance() -> VkInstance = 0;
  virtual auto GetSurface() -> VkSurfaceKHR = 0;
  virtual auto GetPhysicalDevice() -> VkPhysicalDevice = 0;
  virtual auto GetDevice() -> VkDevice = 0;
  virtual auto GetQueueFamilyIndices() -> const QueueFamilyIndices & = 0;
  virtual auto GetQueue(EQueueType) -> VkQueue = 0;
  virtual auto QuerySwapchainSupportDetails(VkPhysicalDevice device)
      -> SwapchainSupportDetails = 0;
  virtual auto GetCapabilities() const noexcept
      -> const DeviceCapabilities & = 0;
};

class IRenderResourceProvider {
public:
  virtual auto GetCurrentPresentTexture() -> TextureHandle = 0;
  virtual auto GetPipeline(PipelineHandle) -> const PipelineResource * = 0;
  virtual auto GetBuffer(BufferHandle) -> const BufferResource * = 0;
  virtual auto GetTexture(TextureHandle) -> const TextureResource * = 0;
  virtual auto GetOrCreateMipStorageView(TextureHandle, uint32_t mipLevel)
      -> VkImageView = 0;
  virtual auto GetSampler(SamplerHandle) -> const SamplerResource * = 0;
  virtual auto GetDescriptorSet(DescriptorSetHandle)
      -> const DescriptorSetResource * = 0;
  virtual auto GetBindlessSet() const -> VkDescriptorSet = 0;
  virtual auto GetBindlessSetLayout() const -> VkDescriptorSetLayout = 0;
  virtual auto GetSceneGlobalSetLayout() const -> VkDescriptorSetLayout = 0;
  virtual auto GetSceneGlobalSet() const -> VkDescriptorSet = 0;
  virtual auto GetSceneGlobalSetHandle() const -> DescriptorSetHandle = 0;
  virtual auto GetCurrentSwapchainImage() const -> VkImage = 0;
  virtual auto GetSwapchainExtent() const -> Extent2D = 0;

  virtual auto GetMaterialSSBOInfo() const
      -> const VkDescriptorBufferInfo & = 0;
  virtual auto GetIndirectSSBOInfo() const
      -> const VkDescriptorBufferInfo & = 0;
  virtual auto GetStaticSSBOInfo() const -> const VkDescriptorBufferInfo & = 0;
  virtual auto GetDynamicSSBOInfo() const -> const VkDescriptorBufferInfo & = 0;
  virtual auto GetGeometriesSSBOInfo() const
      -> const VkDescriptorBufferInfo & = 0;
  virtual auto GetAttributesSSBOInfo() const
      -> const VkDescriptorBufferInfo & = 0;
  virtual auto GetIndicesSSBOInfo() const -> const VkDescriptorBufferInfo & = 0;

  virtual uint32_t GetCurrentFrameIndex() = 0;
  virtual uint32_t GetLastCompletedFrameIndex() = 0;
};

} // namespace avalon::rhi
