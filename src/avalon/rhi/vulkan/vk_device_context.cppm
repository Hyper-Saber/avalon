module;
#include <cstring>
#include <debug/assert.hpp>
#include <expected>
#include <optional>
#include <set>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:device_context;

import avalon.core;
import :types;
import :utils;

namespace avalon::rhi {

#ifndef NDEBUG
bool enableValidation = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    avalon::Error("[VKVL]: {}", pCallbackData->pMessage);
  } else if (messageSeverity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    avalon::Warn("[VKVL]: {}", pCallbackData->pMessage);
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    avalon::Info("[VKVL]: {}", pCallbackData->pMessage);
  } else {
    avalon::Debug("[VKVL]: {}", pCallbackData->pMessage);
  }
  return VK_FALSE;
}

// auto GetSurfaceExtension() -> Array<const char *> {
//   Array<const char *> extensions{VK_KHR_SURFACE_EXTENSION_NAME};
//
// #ifdef VK_USE_PLATFORM_WAYLAND_KHR
//   extensions.PushBack(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
// #elif VK_USE_PLATFORM_XCB_KHR
//   extensions.PushBack(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
// #elif defined(VK_USE_PLATFORM_WIN32_KHR)
//   extensions.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
// #endif
//
//   return extensions;
// }
//
auto GetSurfaceExtension() -> Array<const char *> {
  // 1. 获取系统支持的所有实例扩展
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  Array<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         availableExtensions.GetData());

  auto isSupported = [&](const char *name) {
    for (const auto &ext : availableExtensions) {
      if (strcmp(ext.extensionName, name) == 0)
        return true;
    }
    return false;
  };

  Array<const char *> extensions;

  if (isSupported(VK_KHR_SURFACE_EXTENSION_NAME))
    extensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);

  if (isSupported("VK_KHR_wayland_surface"))
    extensions.PushBack("VK_KHR_wayland_surface");

  if (isSupported("VK_KHR_xcb_surface"))
    extensions.PushBack("VK_KHR_xcb_surface");

  if (isSupported("VK_KHR_xlib_surface"))
    extensions.PushBack("VK_KHR_xlib_surface");

  return extensions;
}

class DeviceContext final : public IDeviceContext,
                            public NonCopyable,
                            public mem::AutoDestroyable<DeviceContext> {
public:
  using mem::IAutoDestroyable::Initialize;

  ~DeviceContext() {
    if (m_device != VK_NULL_HANDLE) {
      vkDestroyDevice(m_device, nullptr);
    }

    if (m_surface != VK_NULL_HANDLE) {
      vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }

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

  auto Initialize(const DeviceConfig &config,
                  const window::NativeWindowInfo &windowInfo)
      -> std::expected<void, ERhiResult> {
    m_config = config;
    return CreateInstance()
#ifndef NDEBUG
        .and_then([&] { return CreateDebugMessenger(); })
#endif // !NDEBUG
        .and_then([&] { return CreateSurface(windowInfo); })
        .and_then([&] { return DiscoverDevices(); })
        .and_then([&] { return PickPhysicalDevice(); })
        .and_then([&] { return CreateLogicalDevice(); });
  }

  auto GetInstance() noexcept -> VkInstance override { return m_instance; }
  auto GetDevice() noexcept -> VkDevice override { return m_device; }
  auto GetPhysicalDevice() noexcept -> VkPhysicalDevice override {
    return m_physicalDevice;
  }
  auto GetSurface() noexcept -> VkSurfaceKHR override { return m_surface; }

  auto GetQueue(EQueueType queueType) -> VkQueue override {
    switch (queueType) {
    case EQueueType::Graphics:
      return m_graphicsQueue;
    case EQueueType::Transfer:
      AVALON_ASSERT(m_config.queueRequirement.isRequireTransfer);
      return m_transferQueue;
    case EQueueType::Compute:
      AVALON_ASSERT(m_config.queueRequirement.isRequireCompute);
      return m_computeQueue;
    case EQueueType::Present:
      AVALON_ASSERT(m_config.queueRequirement.isRequirePresent);
      return m_presentQueue;
    }
  }

  auto GetCapabilities() const noexcept -> const DeviceCapabilities & override {
    return m_capabilities;
  }

  auto GetQueueFamilyIndices() noexcept -> const QueueFamilyIndices & override {
    return m_queueFamilyIndices;
  }

  auto QuerySwapchainSupportDetails(VkPhysicalDevice device)
      -> SwapchainSupportDetails override {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount,
                                         nullptr);
    details.surfaceFormats.Resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount,
                                         details.surfaceFormats.GetData());

    uint32_t modeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount,
                                              nullptr);
    details.presentModes.Resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount,
                                              details.presentModes.GetData());

    return details;
  }

private:
  auto CreateInstance() -> std::expected<void, ERhiResult> {
    bool isRequirePresent = m_config.queueRequirement.isRequirePresent;
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "avalon",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    Array<const char *> extensions = {};
    if (isRequirePresent)
      extensions = GetSurfaceExtension();
    Array<const char *> layers;

#ifndef NDEBUG
    if (enableValidation) {
      layers.PushBack("VK_LAYER_KHRONOS_validation");
      extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#endif // !NDEBUG

    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.GetSize()),
        .ppEnabledLayerNames = layers.GetData(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.GetSize()),
        .ppEnabledExtensionNames = extensions.GetData(),
    };

    auto result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
      avalon::Error(
          "[Vulkan]: Failed to create instance! Check if your drivers "
          "support Vulkan. Error code: {}",
          static_cast<int>(result));
      return std::unexpected(HandleVkError(result));
    }

    return {};
  }

  auto CreateDebugMessenger() -> std::expected<void, ERhiResult> {

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
      Warn("[Vulkan]: Failed to load vkCreateDebugUtilsMessengerEXT (Extension "
           "missing or not enabled!)");
      return {};
    }

    auto result = func(m_instance, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS) {
      avalon::Error("[Vulkan]: Failed to create debug messenger!");
      return std::unexpected(HandleVkError(result));
    }

    avalon::Debug("[Vulkan]: Debug messenger created.");
    return {};
  }

  auto CreateSurface(const window::NativeWindowInfo &windowInfo)
      -> std::expected<void, ERhiResult> {

    VkResult result = VK_ERROR_INITIALIZATION_FAILED;
    if constexpr (platform::kIsLinux) {
      if (windowInfo.api == window::NativeWindowApi::Wayland) {
        VkWaylandSurfaceCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = static_cast<wl_display *>(windowInfo.wayland.display),
            .surface = static_cast<wl_surface *>(windowInfo.wayland.surface),
        };
        result = vkCreateWaylandSurfaceKHR(m_instance, &createInfo, nullptr,
                                           &m_surface);
      }

      else if (windowInfo.api == window::NativeWindowApi::Xcb) {
        VkXcbSurfaceCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
            .connection =
                static_cast<xcb_connection_t *>(windowInfo.xcb.connection),
            .window = static_cast<xcb_window_t>(windowInfo.xcb.window),
        };
        result =
            vkCreateXcbSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
      }
    } else if constexpr (platform::kIsWindows) {
    }

    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create surface!");
      return std::unexpected(HandleVkError(result));
    }

    return {};
  }

  auto DiscoverDevices() -> std::expected<void, ERhiResult> {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    m_availablePhysicalDevices.Resize(count);
    vkEnumeratePhysicalDevices(m_instance, &count,
                               m_availablePhysicalDevices.GetData());
    return {};
  }

  auto PickPhysicalDevice() -> std::expected<void, ERhiResult> {

    for (auto device : m_availablePhysicalDevices) {
      if (IsDeviceSuitable(device)) {
        m_physicalDevice = device;
        PopulateCapabilities();
        return {};
      }
    }
    avalon::Error("Vulkan: Failed to find a suitable GPU!");
    return std::unexpected(ERhiResult::DeviceLost);
  }

  bool IsDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    m_queueFamilyIndices = FindQueueFamilies(device, m_config.queueRequirement);
    bool extensionsSupported = CheckDeviceExtensionSupport(device, m_config);

    return m_queueFamilyIndices.IsComplete(m_config.queueRequirement) &&
           deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  }

  void PopulateCapabilities() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
    m_capabilities.limits.minUniformBufferOffsetAlignment =
        properties.limits.minUniformBufferOffsetAlignment;
    m_capabilities.limits.minStorageBufferOffsetAlignment =
        properties.limits.minStorageBufferOffsetAlignment;
    m_capabilities.limits.maxSamplerAnisotroy =
        properties.limits.maxSamplerAnisotropy;

    IRhi::capabilities = m_capabilities;
  }

  auto CreateLogicalDevice() -> std::expected<void, ERhiResult> {

    float queueamilyPriority = 1.0f;
    Array<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_queueFamilyIndices.graphicsFamily.value(),
    };

    if (m_config.queueRequirement.isRequirePresent) {
      uniqueQueueFamilies.insert(m_queueFamilyIndices.presentFamily.value());
    }
    if (m_config.queueRequirement.isRequireTransfer) {
      uniqueQueueFamilies.insert(m_queueFamilyIndices.transferFamily.value());
    }
    if (m_config.queueRequirement.isRequireCompute) {
      uniqueQueueFamilies.insert(m_queueFamilyIndices.computeFamily.value());
    }

    for (auto queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo createInfo{
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queueFamily,
          .queueCount = 1,
          .pQueuePriorities = &queueamilyPriority,
      };
      queueCreateInfos.PushBack(createInfo);
    }

    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features13,

        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        // .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features12,
        .features = m_config.features,
    };

    if constexpr (debug::kIsDebug) {
      m_config.extensions.PushBack("VK_GOOGLE_user_type");
    }

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .queueCreateInfoCount =
            static_cast<uint32_t>(queueCreateInfos.GetSize()),
        .pQueueCreateInfos = queueCreateInfos.GetData(),
        .enabledExtensionCount =
            static_cast<uint32_t>(m_config.extensions.GetSize()),
        .ppEnabledExtensionNames = m_config.extensions.GetData(),
        .pEnabledFeatures = nullptr,
    };

    auto result =
        vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
      avalon::Error("[Vulkan]: Failed to create logical device!");
      return std::unexpected(HandleVkError(result));
    }

    vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0,
                     &m_graphicsQueue);
    if (m_config.queueRequirement.isRequirePresent)
      vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0,
                       &m_presentQueue);
    if (m_config.queueRequirement.isRequireTransfer) {
      if (m_queueFamilyIndices.transferFamily.has_value())
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(),
                         0, &m_transferQueue);
      else {
        Debug("[Vulkan]: Transfer queue not found. Using graphics queue "
              "instead.");
        m_transferQueue = m_graphicsQueue;
      }
    }
    if (m_config.queueRequirement.isRequireCompute)
      vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily.value(), 0,
                       &m_computeQueue);

    avalon::Debug("[Vulkan]: Logical device created.");
    return {};
  }

  QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device,
                                       QueueRequirement requirement) {
    QueueFamilyIndices indices;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    Array<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count,
                                             properties.GetData());

    uint32_t i = 0;
    for (const auto &property : properties) {
      if (requirement.isRequireGraphics &&
          property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
      }
      if (requirement.isRequireCompute &&
          property.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        indices.computeFamily = i;
      }
      if (requirement.isRequireTransfer) {
        if ((property.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(property.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
          indices.transferFamily = i;
        } else if (!indices.transferFamily.has_value() &&
                   (property.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
          indices.transferFamily = i;
        }
      }
      if (requirement.isRequirePresent) {
        VkBool32 isSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface,
                                             &isSupported);
        if (isSupported == VK_TRUE) {
          indices.presentFamily = i;
        }
      }

      if (indices.IsComplete(requirement)) {
        break;
      }
      i++;
    }

    return indices;
  }

  bool CheckDeviceExtensionSupport(VkPhysicalDevice device,
                                   const DeviceConfig &config) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    Array<VkExtensionProperties> availableExtensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count,
                                         availableExtensions.GetData());

    uint32_t foundCount = 0;

    for (auto required : config.extensions) {
      bool found = false;
      auto view = StringView(required);

      for (const auto &extension : availableExtensions) {
        if (view == StringView(extension.extensionName)) {
          found = true;
          foundCount++;
          break;
        }
      }

      if (!found)
        return false;
    }

    return foundCount == config.extensions.GetSize();
  }

private:
  DeviceConfig m_config;

  VkInstance m_instance{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};
  VkSurfaceKHR m_surface{VK_NULL_HANDLE};
  Array<VkPhysicalDevice> m_availablePhysicalDevices;
  VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
  VkDevice m_device{VK_NULL_HANDLE};
  VkQueue m_graphicsQueue{VK_NULL_HANDLE};
  VkQueue m_presentQueue{VK_NULL_HANDLE};
  VkQueue m_computeQueue{VK_NULL_HANDLE};
  VkQueue m_transferQueue{VK_NULL_HANDLE};

  DeviceCapabilities m_capabilities;

  QueueFamilyIndices m_queueFamilyIndices;
};

} // namespace avalon::rhi
