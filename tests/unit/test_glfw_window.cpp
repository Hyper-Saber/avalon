#include <format>
#include <iostream>
#include <string_view>

import avalon.core;
import avalon.window; // 导入窗口接口定义
import test.utils;

using namespace test_config;

namespace test_config {
// 这里的名字要和你的 xmake.lua 中定义的 target 名字一致
static constexpr std::string_view kWindowPluginName = "libavalon.window.glfw";
} // namespace test_config

// 1. 测试基础加载逻辑
void test_window_plugin_loading() {
  const std::string fullPath = std::format(
      "plugins/{}{}", test_config::kWindowPluginName, avalon::kPluginExtension);

  auto result = avalon::LoadPlugin<avalon::window::IWindow>(fullPath);

  expect(result.has_value(), "GlfwWindow plugin should load successfully");
  if (result) {
    expect(result.value().get() != nullptr, "Window instance pointer valid");
  }
}

// 2. 测试初始化与窗口属性
void test_window_initialization() {
  const std::string fullPath = std::format(
      "{}{}", test_config::kWindowPluginName, avalon::kPluginExtension);
  auto result = avalon::LoadPlugin<avalon::window::IWindow>(fullPath);

  if (!result)
    return;
  auto &window = result.value();

  // 业务初始化
  avalon::window::WindowProps props{
      .title = "Test Window", .width = 800, .height = 600};

  auto initRes = window->Initialize(props);

  // 注意：如果是在没有显示器的 Linux CI 上运行，这里可能会返回 WindowError
  // 我们可以根据这个结果来做断言
  if (initRes.has_value()) {
    expect(window->ShouldClose() == false, "New window should not be closing");

    uint32_t w, h;
    window->GetFrameBufferSize(w, h);
    expect(w > 0 && h > 0, "Window should have valid dimensions");

    // 测试 Native 信息
    auto native = window->GetNativeInfo();
    if constexpr (avalon::kIsLinux) {
      bool is_valid_api =
          (native.api == avalon::window::NativeWindowApi::Xcb ||
           native.api == avalon::window::NativeWindowApi::Wayland);
      expect(is_valid_api, "Linux native API should be XCB or Wayland");
    }
  } else {
    std::cout << "[Skip] Window creation failed (likely Headless environment)"
              << std::endl;
  }
}

// 3. 测试最小化状态逻辑
void test_window_minimize_logic() {
  const std::string fullPath = std::format(
      "{}{}", test_config::kWindowPluginName, avalon::kPluginExtension);
  auto result = avalon::LoadPlugin<avalon::window::IWindow>(fullPath);
  if (result && result.value()->Initialize({}).has_value()) {
    auto &window = result.value();
    // 初始状态不应该是最小化的
    expect(window->IsMinimized() == false,
           "Initial window should not be minimized");
  }
}

int main() {
  std::cout << "--- Starting Avalon GlfwWindow Plugin Tests ---" << std::endl;

  test_window_plugin_loading();
  test_window_initialization();
  test_window_minimize_logic();

  std::cout << "--- All Window Tests Completed ---" << std::endl;
  return 0;
}
