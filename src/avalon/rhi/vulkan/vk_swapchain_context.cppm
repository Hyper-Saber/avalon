module;
#include <algorithm>
#include <expected>
#include <limits>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:swapchain_context;

import avalon.core;
import :types;
import :utils;

namespace avalon::rhi {
class SwapchainContext final : public NonCopyable,
                               public mem::AutoDestroyable<SwapchainContext> {
public:
  using mem::IAutoDestroyable::Initialize;

  explicit SwapchainContext(IDeviceContext &context)
      : m_deviceContext(context) {}

  ~SwapchainContext() { CleanupSwapchain(); }

  auto Initialize(uint32_t width, uint32_t height)
      -> std::expected<void, ERhiResult> {
    return CreateSwapchain(width, height);
  }

  auto GetSwapchain() noexcept { return m_swapchain; }
  auto GetImageFormat() noexcept { return m_imageFormat; }
  auto GetImageViews() noexcept -> const Array<VkImageView> & {
    return m_imageViews;
  }

  auto GetImageCount() const noexcept { return m_images.GetSize(); }

  auto GetExtent() noexcept { return m_extent; }
  auto GetFrameBuffer(uint32_t index) noexcept { return m_frameBuffers[index]; }
  auto GetFrameBuffers() noexcept
      -> const Array<Handle<FrameBufferResource>> & {
    return m_frameBuffers;
  }

  void
  SetFrameBuffers(Array<Handle<FrameBufferResource>> frameBuffers) noexcept {
    m_frameBuffers = std::move(frameBuffers);
  }

  auto RecreateSwapchain(uint32_t width, uint32_t height)
      -> std::expected<void, ERhiResult> {
    vkDeviceWaitIdle(m_deviceContext.GetDevice());

    CleanupSwapchainResources();

    return CreateSwapchain(width, height);
  }

private:
  auto CreateSwapchain(uint32_t width, uint32_t height)
      -> std::expected<void, ERhiResult> {

    auto supportDetails = m_deviceContext.QuerySwapchainSupportDetails(
        m_deviceContext.GetPhysicalDevice());

    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
    if (supportDetails.capabilities.maxImageCount > 0 &&
        imageCount > supportDetails.capabilities.maxImageCount) {
      imageCount = supportDetails.capabilities.maxImageCount;
    }
    auto surfaceFormat = ChooseSwapSurfaceFormat(supportDetails.surfaceFormats);
    auto presentMode = ChooseSwapPresentMode(supportDetails.presentModes);
    auto extent = ChooseSwapExtent(supportDetails.capabilities, width, height);
    m_imageFormat = surfaceFormat.format;
    m_extent = extent;

    auto allQueueFamilyIndices = m_deviceContext.GetQueueFamilyIndices();

    std::array<uint32_t, 2> queueFamilyIndices = {
        allQueueFamilyIndices.graphicsFamily.value(),
        allQueueFamilyIndices.presentFamily.value()};

    auto oldSwapchain = m_swapchain;

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_deviceContext.GetSurface(),
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = supportDetails.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain,
    };

    if (allQueueFamilyIndices.graphicsFamily !=
        allQueueFamilyIndices.presentFamily) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    auto result = vkCreateSwapchainKHR(m_deviceContext.GetDevice(), &createInfo,
                                       nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create swapchain! Error code: {}.",
                    ToView(result));
      return std::unexpected(HandleVkError(result));
    }

    if (oldSwapchain != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(m_deviceContext.GetDevice(), oldSwapchain, nullptr);
    }

    vkGetSwapchainImagesKHR(m_deviceContext.GetDevice(), m_swapchain,
                            &imageCount, nullptr);
    m_images.Clear();
    m_images.Resize(imageCount);
    vkGetSwapchainImagesKHR(m_deviceContext.GetDevice(), m_swapchain,
                            &imageCount, m_images.GetData());

    m_imageViews.Clear();
    m_imageViews.Resize(imageCount);
    uint32_t i = 0;
    for (const auto image : m_images) {
      auto view =
          CreateImageView(image, m_imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
      if (view == VK_NULL_HANDLE) {
        return std::unexpected(ERhiResult::SwapchainOutOfDate);
      }
      m_imageViews[i++] = view;
      FrameBufferCreateInfo info{
          .width = m_extent.width,
          .height = m_extent.height,
      };
    }

    Debug("[Vulkan]: Swapchain created.");
    return {};
  }

  auto
  ChooseSwapPresentMode(const Array<VkPresentModeKHR> &availablePresentModes)
      -> VkPresentModeKHR {
    for (const auto &mode : availablePresentModes) {
      if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return mode;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  auto
  ChooseSwapSurfaceFormat(const Array<VkSurfaceFormatKHR> &availableFormats)
      -> VkSurfaceFormatKHR {
    for (const auto &format : availableFormats) {
      if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
          format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return format;
      }
    }
    return availableFormats[0];
  }

  auto ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capacities,
                        uint32_t width, uint32_t height) -> VkExtent2D {
    if (capacities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      return capacities.currentExtent;
    } else {
      VkExtent2D actualExtent = {width, height};
      actualExtent.width = std::max(
          capacities.minImageExtent.width,
          std::min(capacities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(
          capacities.minImageExtent.height,
          std::min(capacities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
    }
  }

  void CleanupSwapchain() {
    CleanupSwapchainResources();
    if (m_swapchain != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(m_deviceContext.GetDevice(), m_swapchain, nullptr);
  }

  void CleanupSwapchainResources() {
    for (const auto view : m_imageViews) {
      vkDestroyImageView(m_deviceContext.GetDevice(), view, nullptr);
    }
  }

  auto CreateImageView(VkImage image, VkFormat format,
                       VkImageAspectFlags aspectFlags) -> VkImageView {

    VkImageViewCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange{
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    VkImageView imageView{VK_NULL_HANDLE};
    auto result = vkCreateImageView(m_deviceContext.GetDevice(), &createInfo,
                                    nullptr, &imageView);
    if (result != VK_SUCCESS) {
      avalon::Error("[Vulkan]: Failed to create image view! Error code: {}",
                    ToView(result));
    }
    return imageView;
  }

  IDeviceContext &m_deviceContext;

  VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
  Array<VkImage> m_images;
  Array<VkImageView> m_imageViews;
  Array<Handle<FrameBufferResource>> m_frameBuffers;
  VkFormat m_imageFormat;
  VkExtent2D m_extent;
};
} // namespace avalon::rhi
