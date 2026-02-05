module;
#include <array>
#include <expected>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

module avalon.rhi;

import avalon.core;
import :vulkan;

namespace avalon::rhi {

#ifndef NDEBUG
bool enableValidation = true;
#endif

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    avalon::Error("Vulkan: {}", pCallbackData->pMessage);
  } else if (messageSeverity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    avalon::Warn("Vulkan: {}", pCallbackData->pMessage);
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    avalon::Info("Vulkan: {}", pCallbackData->pMessage);
  } else {
    avalon::Debug("Vulkan: {}", pCallbackData->pMessage);
  }
  return VK_FALSE;
}

static auto HandleVkError(VkResult result) -> ERhiError {
  switch (result) {
  case VK_ERROR_DEVICE_LOST:
    return ERhiError::DeviceLost;
    break;
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return ERhiError::OutOfMemory;
    break;
  case VK_ERROR_SURFACE_LOST_KHR:
    return ERhiError::SurfaceLost;
    break;
  default:
    return ERhiError::Unknown;
  }
}

VkContext::~VkContext() { InternalCleanup(); }

auto VkContext::OnLoad() -> std::expected<void, EStatusCode> { return {}; };

auto VkContext::Initialize(const window::NativeWindowInfo &inWindowInfo,
                           uint32_t width, uint32_t height)
    -> std::expected<void, EStatusCode> {
  return CreateInstance()
#ifndef NDEBUG
      .and_then([this] { return CreateDebugMessenger(); })
#endif // !NDEBUG
      .and_then([this, inWindowInfo] { return CreateSurface(inWindowInfo); })
      .and_then([this] { return PickPhysicalDevice(); })
      .and_then([this] { return CreateLogicalDevice(); })
      .and_then(
          [this, width, height] { return CreateSwapchain(width, height); })
      .and_then([this] { return CreateSwapchainImageViews(); })
      .and_then([this] { return CreateRenderPass(); })
      .and_then([this] { return CreateFrameBuffers(); })
      .and_then([this] { return CreateCommandPool(); })
      .and_then([this] { return CreateCommandBuffer(); })
      .and_then([this] { return CreateSyncObjects(); })
      .transform_error([](ERhiError ERhiErr) {
        avalon::Error("RHI: Vulkan context initialization aborted.");
        return EStatusCode::NotSupported;
      });
}

auto VkContext::GetRequiredExtensions() -> std::vector<const char *> {
  std::vector<const char *> extensions{VK_KHR_SURFACE_EXTENSION_NAME};

// if constexpr (kIsLinux) {
//   extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
//   extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
// } else if constexpr (kIsWindows) {
//   extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
// }
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

  return extensions;
}

auto VkContext::CreateInstance() -> std::expected<void, ERhiError> {
  VkApplicationInfo appInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "avalon",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };

  std::vector<const char *> extensions = GetRequiredExtensions();
  std::vector<const char *> layers;

#ifndef NDEBUG
  if (enableValidation) {
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
#endif // !NDEBUG

  VkInstanceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  auto result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create instance! Check if your drivers "
                  "support Vulkan {}.",
                  VK_VERSION_MAJOR(VK_API_VERSION_1_3));
    return std::unexpected(HandleVkError(result));
  }

  return {};
}

void VkContext::InternalCleanup() {
  vkDeviceWaitIdle(m_device);
  if (m_renderFinishedSemaphore != VK_NULL_HANDLE)
    vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
  if (m_imageAvailableSemaphore != VK_NULL_HANDLE)
    vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
  if (m_inflightFence != VK_NULL_HANDLE)
    vkDestroyFence(m_device, m_inflightFence, nullptr);

  if (m_commandPool != VK_NULL_HANDLE)
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  if (m_renderPass != VK_NULL_HANDLE)
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

  CleanupSwapchain();

  if (m_device != VK_NULL_HANDLE) {
    vkDestroyDevice(m_device, nullptr);
  }

  if (m_surface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

#ifndef NDEBUG
  if (enableValidation && m_debugMessenger != VK_NULL_HANDLE) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func)
      func(m_instance, m_debugMessenger, nullptr);
  }
#endif // !NDEBUG
  if (m_instance != VK_NULL_HANDLE)
    vkDestroyInstance(m_instance, nullptr);
}

void VkContext::CleanupSwapchain() {
  CleanupSwapchainResources();
  if (m_swapchain != VK_NULL_HANDLE)
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VkContext::CleanupSwapchainResources() {
  for (auto frameBuffer : m_frameBuffers) {
    vkDestroyFramebuffer(m_device, frameBuffer, nullptr);
  }
  m_frameBuffers.clear();

  for (auto imageView : m_swapchainImageViews) {
    vkDestroyImageView(m_device, imageView, nullptr);
  }
  m_swapchainImageViews.clear();
}

auto VkContext::CreateDebugMessenger() -> std::expected<void, ERhiError> {
  avalon::Info("Vulkan: {}", "Creating debug messenger");

  VkDebugUtilsMessengerCreateInfoEXT createInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback,
      .pUserData = nullptr,
  };

  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
  if (!func) {
  avalon:
    Warn("Vulkan: Failed to load vkCreateDebugUtilsMessengerEXT (Extension "
         "missing or not enabled!)");
    return {};
  }

  auto result = func(m_instance, &createInfo, nullptr, &m_debugMessenger);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create debug messenger!");
    return std::unexpected(HandleVkError(result));
  }

  return {};
}

auto VkContext::PickPhysicalDevice() -> std::expected<void, ERhiError> {
  avalon::Info("Vulkan: {}", "Picking physical device");

  uint32_t count = 0;
  vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

  for (auto device : devices) {
    if (IsDeviceSuitable(device)) {
      m_physicalDevice = device;
      return {};
    }
  }
  avalon::Error("Vulkan: Failed to find a suitable GPU!");
  return std::unexpected(ERhiError::DeviceLost);
}

bool VkContext::IsDeviceSuitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  auto indices = FindQueueFamilies(device);
  bool extensionsSupported = CheckDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    auto swapChainSupport = QuerySwapchainSupportDetails(device);
    swapChainAdequate = !swapChainSupport.surfaceFormats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  return swapChainAdequate && indices.isComplete() &&
         deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

auto VkContext::CreateSurface(const window::NativeWindowInfo &inWindowInfo)
    -> std::expected<void, ERhiError> {

  VkResult result = VK_ERROR_INITIALIZATION_FAILED;
  if constexpr (kIsLinux) {
    if (inWindowInfo.api == window::NativeWindowApi::Wayland) {
      VkWaylandSurfaceCreateInfoKHR createInfo{
          .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
          .display = static_cast<wl_display *>(inWindowInfo.wayland.display),
          .surface = static_cast<wl_surface *>(inWindowInfo.wayland.surface),
      };
      result = vkCreateWaylandSurfaceKHR(m_instance, &createInfo, nullptr,
                                         &m_surface);
    }

    else if (inWindowInfo.api == window::NativeWindowApi::Xcb) {
      VkXcbSurfaceCreateInfoKHR createInfo{
          .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
          .connection =
              static_cast<xcb_connection_t *>(inWindowInfo.xcb.connection),
          .window = static_cast<xcb_window_t>(inWindowInfo.xcb.window),
      };
      result =
          vkCreateXcbSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
    }
  } else if constexpr (kIsWindows) {
    //
    //   else if (info.api == window::NativeWindowApi::Win32) {
    // #ifdef VK_USE_PLATFORM_WIN32
    //     VkWin32SurfaceCreateInfoKHR createInfo{
    //         .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    //         .hinstance = static_cast<HINSTANCE>(info.win32.hinstance),
    //         .hwnd = static_cast<HWND>(info.win32.hwnd),
    //     };
    //
    //     result =
    //         vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr,
    //         &m_surface);
    // #endif // VK_USE_PLATFORM_WIN32
    //   }
  }

  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create surface!");
    return std::unexpected(HandleVkError(result));
  }

  return {};
}

auto VkContext::CreateLogicalDevice() -> std::expected<void, ERhiError> {
  avalon::Info("Vulkan: {}", "Creating logical device");

  QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

  float queueamilyPriority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  for (auto queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queueamilyPriority,
    };
    queueCreateInfos.push_back(createInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &deviceFeatures,
  };

  auto result =
      vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create logical device!");
    return std::unexpected(HandleVkError(result));
  }

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0,
                   &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

  return {};
}

QueueFamilyIndices VkContext::FindQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties> properties(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

  uint32_t i = 0;
  for (const auto &property : properties) {

    if (property.queueCount > 0 &&
        property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
    if (property.queueCount > 0 && presentSupport) {
      indices.presentFamily = i;
    }
    if (indices.isComplete()) {
      break;
    }
    i++;
  }

  return indices;
}

bool VkContext::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());
  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(std::string(extension.extensionName));
  }

  if (requiredExtensions.empty()) {
    return true;
  }
  return false;
}

auto VkContext::CreateSwapchain(uint32_t width, uint32_t height)
    -> std::expected<void, ERhiError> {

  auto supportDetails = QuerySwapchainSupportDetails(m_physicalDevice);

  uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
  if (supportDetails.capabilities.maxImageCount > 0 &&
      imageCount > supportDetails.capabilities.maxImageCount) {
    imageCount = supportDetails.capabilities.maxImageCount;
  }
  auto surfaceFormat = ChooseSwapSurfaceFormat(supportDetails.surfaceFormats);
  auto presentMode = ChooseSwapPresentMode(supportDetails.presentModes);
  auto extent = ChooseSwapExtent(supportDetails.capabilities, width, height);
  m_swapchainImageFormat = surfaceFormat.format;
  m_swapchainExtent = extent;

  auto indices = FindQueueFamilies(m_physicalDevice);
  std::array<uint32_t, 2> queueFamilyIndices = {indices.graphicsFamily.value(),
                                                indices.presentFamily.value()};

  auto oldSwapchain = m_swapchain;

  VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = m_surface,
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

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  auto result =
      vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create swapchain!");
    return std::unexpected(HandleVkError(result));
  }

  if (oldSwapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
  }

  vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
  m_swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount,
                          m_swapchainImages.data());

  return {};
}

auto VkContext::RecreateSwapchain(uint32_t width, uint32_t height)
    -> std::expected<void, EStatusCode> {
  vkDeviceWaitIdle(m_device);

  CleanupSwapchainResources();

  return CreateSwapchain(width, height)
      .and_then([this] { return CreateSwapchainImageViews(); })
      .and_then([this] { return CreateFrameBuffers(); })
      .transform_error([](ERhiError erhiErr) {
        switch (erhiErr) {
        case ERhiError::OutOfMemory:
          return EStatusCode::OutOfMemory;
          break;
        case ERhiError::SurfaceLost:
          return EStatusCode::WindowError;
          break;
        case ERhiError::DeviceLost:
          return EStatusCode::DeviceLost;
        default:
          return EStatusCode::RhiUpdateFailed;
        }
      });
}

auto VkContext::QuerySwapchainSupportDetails(VkPhysicalDevice device)
    -> SwapchainSupportDetails {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount,
                                       nullptr);
  details.surfaceFormats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount,
                                       details.surfaceFormats.data());

  uint32_t modeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount,
                                            nullptr);
  details.presentModes.resize(modeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount,
                                            details.presentModes.data());

  return details;
}

auto VkContext::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes)
    -> VkPresentModeKHR {
  for (const auto &mode : availablePresentModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

auto VkContext::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats)
    -> VkSurfaceFormatKHR {
  for (const auto &format : availableFormats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return availableFormats[0];
}

auto VkContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capacities,
                                 uint32_t width, uint32_t height)
    -> VkExtent2D {
  if (capacities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capacities.currentExtent;
  } else {
    VkExtent2D actualExtent = {width, height};
    actualExtent.width =
        std::max(capacities.minImageExtent.width,
                 std::min(capacities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(
        capacities.minImageExtent.height,
        std::min(capacities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

auto VkContext::CreateSwapchainImageViews() -> std::expected<void, ERhiError> {
  m_swapchainImageViews.reserve(m_swapchainImages.size());

  for (const auto &image : m_swapchainImages) {
    auto res = CreateImageView(image, m_swapchainImageFormat,
                               VK_IMAGE_ASPECT_COLOR_BIT);
    if (res.has_value()) {
      m_swapchainImageViews.push_back(res.value());
    } else
      return std::unexpected(res.error());
  }
  return {};
}

auto VkContext::CreateImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectFlags)
    -> std::expected<VkImageView, ERhiError> {

  VkImageViewCreateInfo createInfo{.sType =
                                       VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
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

  VkImageView imageView;
  auto result = vkCreateImageView(m_device, &createInfo, nullptr, &imageView);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create image view!");
    return std::unexpected(HandleVkError(result));
  }
  return imageView;
}

auto VkContext::CreateRenderPass() -> std::expected<void, ERhiError> {
  VkAttachmentDescription colorAttachment{
      .format = m_swapchainImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  std::array<VkAttachmentDescription, 1> attachments{
      colorAttachment,
  };

  VkAttachmentReference colorAttachmentRef{
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
  };

  std::array<VkSubpassDescription, 1> subpasses{subpass};

  VkRenderPassCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = subpasses.data(),
  };

  auto result =
      vkCreateRenderPass(m_device, &createInfo, nullptr, &m_renderPass);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create render pass!");
    return std::unexpected(HandleVkError(result));
  }
  return {};
}

auto VkContext::CreateCommandPool() -> std::expected<void, ERhiError> {
  auto indices = FindQueueFamilies(m_physicalDevice);
  VkCommandPoolCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = indices.graphicsFamily.value(),
  };

  auto result =
      vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create command pool!");
    return std::unexpected(HandleVkError(result));
  }

  return {};
}

auto VkContext::CreateCommandBuffer() -> std::expected<void, ERhiError> {
  m_commandBuffers.resize(m_swapchainImageViews.size());
  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())};

  auto result = vkAllocateCommandBuffers(m_device, &allocateInfo,
                                         m_commandBuffers.data());
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to allocate command buffers!");
    return std::unexpected(HandleVkError(result));
  }

  return {};
}

auto VkContext::CreateFrameBuffers() -> std::expected<void, ERhiError> {
  m_frameBuffers.resize(m_swapchainImageViews.size());
  int i = 0;
  for (auto imageView : m_swapchainImageViews) {
    VkImageView attachments[] = {imageView};

    VkFramebufferCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = m_swapchainExtent.width,
        .height = m_swapchainExtent.height,
        .layers = 1,
    };
    auto result = vkCreateFramebuffer(m_device, &createInfo, nullptr,
                                      &m_frameBuffers[i++]);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create framebuffer!");
      return std::unexpected(HandleVkError(result));
    }
  }

  return {};
}

auto VkContext::CreateSyncObjects() -> std::expected<void, ERhiError> {
  VkSemaphoreCreateInfo semaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  auto result = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr,
                                  &m_imageAvailableSemaphore);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create sync objects!");
    return std::unexpected(HandleVkError(result));
  }

  result = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr,
                             &m_renderFinishedSemaphore);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create sync objects!");
    return std::unexpected(HandleVkError(result));
  }

  result = vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_inflightFence);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create sync objects!");
    return std::unexpected(HandleVkError(result));
  }
  return {};
}

auto VkContext::BeginFrame() -> std::expected<void, ERhiError> {
  vkWaitForFences(m_device, 1, &m_inflightFence, VK_TRUE, UINT64_MAX);
  uint32_t imageIndex;
  auto result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                      m_imageAvailableSemaphore, VK_NULL_HANDLE,
                                      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    avalon::Warn("Vulkan: Swapchain is out of date!");
    return std::unexpected(ERhiError::SwapchainOutOfDate);
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    avalon::Error("Vulkan: Failed to acquire swapchain image!");
    return std::unexpected(HandleVkError(result));
  }

  m_currentImageIndex = imageIndex;
  m_currentCommandBuffer = m_commandBuffers[m_currentImageIndex];

  vkResetFences(m_device, 1, &m_inflightFence);

  vkResetCommandBuffer(m_commandBuffers[imageIndex], 0);
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  result = vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to begin recording command buffer!");
    return std::unexpected(HandleVkError(result));
  }

  VkClearValue clearColor = {{{.1f, .1f, .1f}}};
  VkRenderPassBeginInfo renderPassBeginInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = m_renderPass,
      .framebuffer = m_frameBuffers[imageIndex],
      .renderArea = {{0, 0}, m_swapchainExtent},
      .clearValueCount = 1,
      .pClearValues = &clearColor,
  };
  vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(m_swapchainExtent.width),
      .height = static_cast<float>(m_swapchainExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  VkRect2D scissor{{0, 0}, m_swapchainExtent};
  vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);
  vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);

  return {};
}

auto VkContext::EndFrame() -> std::expected<void, ERhiError> {
  vkCmdEndRenderPass(m_commandBuffers[m_currentImageIndex]);

  auto result = vkEndCommandBuffer(m_commandBuffers[m_currentImageIndex]);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to record command buffer!");
    return std::unexpected(HandleVkError(result));
  }

  std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_imageAvailableSemaphore,
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &m_commandBuffers[m_currentImageIndex],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_renderFinishedSemaphore,
  };

  result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inflightFence);
  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to submit queue!");
    return std::unexpected(HandleVkError(result));
  }

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_renderFinishedSemaphore,
      .swapchainCount = 1,
      .pSwapchains = &m_swapchain,
      .pImageIndices = &m_currentImageIndex,
  };

  result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return std::unexpected(ERhiError::SwapchainOutOfDate);
  }

  else if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to present queue!");
    return std::unexpected(HandleVkError(result));
  }
  return {};
}

} // namespace avalon::rhi
