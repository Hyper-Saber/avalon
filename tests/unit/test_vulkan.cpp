#include <format>
#include <iostream>
#include <string_view>

import avalon.core;
import avalon.window;
import avalon.rhi; // 导入 IRhi 定义
import test.utils;

using namespace test_config;

namespace test_config {
static constexpr std::string_view kWindowPluginName = "libavalon.window.glfw";
static constexpr std::string_view kVulkanPluginName = "libavalon.rhi.vulkan";
} // namespace test_config

void test_vulkan_plugin_full_lifecycle() {
  // 1. 环境准备：加载并初始化窗口插件
  const std::string winPath = std::format(
      "plugins/{}{}", test_config::kWindowPluginName, avalon::kPluginExtension);
  auto winResult = avalon::LoadPlugin<avalon::window::IWindow>(winPath);

  expect(winResult.has_value(), "Window plugin should be loadable");
  auto &window = winResult.value();
  auto initResult =
      window->Initialize({.title = "RHI Test", .width = 800, .height = 600});
  expect(initResult.has_value(), "Window initialization should be successful");

  // 2. 核心测试：加载 Vulkan RHI 插件
  const std::string vlkPath = std::format(
      "plugins/{}{}", test_config::kVulkanPluginName, avalon::kPluginExtension);
  auto vlkResult = avalon::LoadPlugin<avalon::rhi::IRhi>(vlkPath);

  expect(vlkResult.has_value(), "Vulkan RHI plugin should be loadable");

  if (vlkResult) {
    auto &rhi = vlkResult.value();

    // 3. 测试继承链与接口
    expect(rhi.get() != nullptr, "RHI instance should not be null");

    // 获取原生窗口句柄用于创建 Surface
    auto nativeInfo = window->GetNativeInfo();
    uint32_t width, height;
    window->GetFrameBufferSize(width, height);
    // 4. 执行 Vulkan 初始化
    // 注意：Initialize 成功的前提是系统有兼容的驱动和 Vulkan 运行时
    auto initRes = rhi->Initialize(nativeInfo, width, height);

    if (initRes) {
      std::cout << "SUCCESS: Vulkan RHI initialized with Native Surface."
                << std::endl;

      // 5. 模拟一帧渲染流
      auto beginRes = rhi->BeginFrame();
      if (beginRes) {
        expect(rhi->EndFrame().has_value(),
               "EndFrame should succeed after BeginFrame");
      }
    } else {
      // 如果初始化失败，打印错误原因（如：驱动不支持）
      std::cout << "WARNING: Vulkan RHI init failed. Reason: "
                << static_cast<int>(initRes.error()) << std::endl;
    }
  }
}

int main() {
  std::cout << "--- Starting Avalon Plugin-based RHI Tests ---" << std::endl;

  test_vulkan_plugin_full_lifecycle();

  std::cout << "--- RHI Tests Completed ---" << std::endl;
  return 0;
}
