module avalon.rhi;
import :vulkan;
import avalon.core;

extern "C" AVALON_RHI_VULKAN_API avalon::IPlugin *CreatePlugin() {
  return new avalon::rhi::VkContext();
}

extern "C" AVALON_RHI_VULKAN_API void DestroyPlugin(avalon::IPlugin *plugin) {
  if (plugin) {
    delete plugin;
  }
}
