module;
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

struct PipelineResource : public mem::AutoDestroyable<PipelineResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkPipeline pipeline{VK_NULL_HANDLE};
  const VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  const Array<VkDescriptorSetLayout> setLayouts;

  PipelineResource(VkDevice device, VkPipeline pipeline,
                   VkPipelineLayout layout,
                   Array<VkDescriptorSetLayout> &&setLayouts)
      : device(device), pipeline(pipeline), pipelineLayout(layout),
        setLayouts(std::move(setLayouts)) {}

  ~PipelineResource() {
    vkDestroyPipeline(device, pipeline, nullptr);

    for (auto setLayout : setLayouts) {
      vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
    }
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

struct TextureResource : public mem::AutoDestroyable<TextureResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkImage image{VK_NULL_HANDLE};
  const VkImageView imageView{VK_NULL_HANDLE};
  const VkDeviceMemory memory{VK_NULL_HANDLE};

  TextureResource(VkDevice device, VkImage image, VkImageView imageView,
                  VkDeviceMemory memory)
      : device(device), image(image), imageView(imageView), memory(memory) {}

  ~TextureResource() {
    vkDestroyImageView(device, imageView, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);
  }
};

struct RenderPassResource : public mem::AutoDestroyable<RenderPassResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkRenderPass renderPass{VK_NULL_HANDLE};
  const Array<AttachmentDescription> AttachmentDescriptions;

  static RenderPassResource Null() {
    return RenderPassResource{VK_NULL_HANDLE, VK_NULL_HANDLE, {}};
  }

  RenderPassResource(VkDevice device, VkRenderPass renderPass,
                     Array<AttachmentDescription> attachmentDescriptions)
      : device(device), renderPass(renderPass),
        AttachmentDescriptions(std::move(attachmentDescriptions)) {}

  ~RenderPassResource() { vkDestroyRenderPass(device, renderPass, nullptr); }
};

struct FrameBufferResource : public mem::AutoDestroyable<FrameBufferResource> {
  const VkDevice device{VK_NULL_HANDLE};
  const VkFramebuffer frameBuffer{VK_NULL_HANDLE};

  FrameBufferResource(VkDevice device, VkFramebuffer buffer)
      : device(device), frameBuffer(buffer) {}

  ~FrameBufferResource() { vkDestroyFramebuffer(device, frameBuffer, nullptr); }
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
};

class IRenderResourceProvider {
public:
  virtual auto GetRenderPass(RenderPassHandle)
      -> const RenderPassResource & = 0;
  virtual auto GetFrameBuffer(ERenderTarget) -> const FrameBufferResource & = 0;
  virtual auto GetPipeline(PipelineHandle) -> const PipelineResource & = 0;
  virtual auto GetBuffer(BufferHandle) -> const BufferResource & = 0;
};

} // namespace avalon::rhi
