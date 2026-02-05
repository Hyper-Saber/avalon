module;

#include <expected>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

module avalon.rhi:vulkan;

import avalon.rhi;
import avalon.core;

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> surfaceFormats;
  std::vector<VkPresentModeKHR> presentModes;
};

namespace avalon::rhi {

class VkContext final : public IRhi {

public:
  ~VkContext() override;
  auto OnLoad() -> std::expected<void, EStatusCode> override;
  auto Initialize(const window::NativeWindowInfo &inWindowInfo, uint32_t width,
                  uint32_t height) -> std::expected<void, EStatusCode> override;

  auto RecreateSwapchain(uint32_t width, uint32_t height)
      -> std::expected<void, EStatusCode> override;
  auto BeginFrame() -> std::expected<void, ERhiError> override;
  auto EndFrame() -> std::expected<void, ERhiError> override;

private:
  void InternalCleanup();

  auto CreateInstance() -> std::expected<void, ERhiError>;
  auto CreateDebugMessenger() -> std::expected<void, ERhiError>;
  auto PickPhysicalDevice() -> std::expected<void, ERhiError>;
  auto CreateSurface(const window::NativeWindowInfo &inWindowInfo)
      -> std::expected<void, ERhiError>;
  auto CreateLogicalDevice() -> std::expected<void, ERhiError>;
  auto CreateSwapchain(uint32_t width, uint32_t height)
      -> std::expected<void, ERhiError>;
  auto CreateSwapchainImageViews() -> std::expected<void, ERhiError>;
  auto CreateRenderPass() -> std::expected<void, ERhiError>;
  auto CreateCommandPool() -> std::expected<void, ERhiError>;
  auto CreateCommandBuffer() -> std::expected<void, ERhiError>;
  auto CreateSyncObjects() -> std::expected<void, ERhiError>;
  auto CreateFrameBuffers() -> std::expected<void, ERhiError>;
  void CleanupSwapchainResources();
  void CleanupSwapchain();

  auto GetRequiredExtensions() -> std::vector<const char *>;
  bool IsDeviceSuitable(VkPhysicalDevice device);
  auto FindQueueFamilies(VkPhysicalDevice device) -> QueueFamilyIndices;
  bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
  auto QuerySwapchainSupportDetails(VkPhysicalDevice device)
      -> SwapchainSupportDetails;
  auto ChooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats)
      -> VkSurfaceFormatKHR;
  auto ChooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes)
      -> VkPresentModeKHR;
  auto ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capacities,
                        uint32_t width, uint32_t height) -> VkExtent2D;
  auto CreateImageView(VkImage image, VkFormat format,
                       VkImageAspectFlags aspectFlags)
      -> std::expected<VkImageView, ERhiError>;

private:
  VkInstance m_instance{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};
  VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
  VkSurfaceKHR m_surface{VK_NULL_HANDLE};
  VkDevice m_device{VK_NULL_HANDLE};
  VkQueue m_graphicsQueue{VK_NULL_HANDLE};
  VkQueue m_presentQueue{VK_NULL_HANDLE};
  VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
  VkFormat m_swapchainImageFormat;
  VkExtent2D m_swapchainExtent;
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  VkRenderPass m_renderPass{VK_NULL_HANDLE};
  VkCommandPool m_commandPool{VK_NULL_HANDLE};
  std::vector<VkCommandBuffer> m_commandBuffers;
  std::vector<VkFramebuffer> m_frameBuffers;
  VkSemaphore m_imageAvailableSemaphore;
  VkSemaphore m_renderFinishedSemaphore;
  VkFence m_inflightFence;
  uint32_t m_currentImageIndex = 0;
  VkCommandBuffer m_currentCommandBuffer{VK_NULL_HANDLE};

  bool m_shouldResize;
};
} // namespace avalon::rhi
