module;
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#ifdef __linux__
#include <X11/Xlib-xcb.h>
#endif

module avalon.window;
import avalon.core;

namespace avalon::window {

class GlfwWindow final : public IWindow {
public:
  ~GlfwWindow() override { InternalCleanup(); }

  auto OnLoad() -> EStatusCode override {
    if (!glfwInit()) {
      avalon::Error("[GlfwWindow]: Failed to initialize GLFW!");
      return EStatusCode::PluginInitializeError;
    }
    return {};
  }

  void SetTitle(StringView title) override {
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwSetWindowTitle(m_window, title.GetData());
  }

  auto Initialize(const WindowProps &props) -> EStatusCode override {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(props.width, props.height, props.title, nullptr,
                                nullptr);

    if (!m_window) {
      avalon::Error("[GlfwWindow]: Failed to create GLFW window!");
      return EStatusCode::WindowError;
    }

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    m_width = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int w,
                                                int h) {
      auto self = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
      self->m_isResized = true;
      self->m_width = static_cast<uint32_t>(w);
      self->m_height = static_cast<uint32_t>(h);
    });

    return {};
  }

  auto GetNativeInfo() -> NativeWindowInfo const override {
    NativeWindowInfo info{};

    auto platform = glfwGetPlatform();
    if constexpr (platform::kIsLinux) {
      if (platform == GLFW_PLATFORM_WAYLAND) {
        info.api = NativeWindowApi::Wayland;
        info.wayland.display = glfwGetWaylandDisplay();
        info.wayland.surface = glfwGetWaylandWindow(m_window);
      } else if (platform == GLFW_PLATFORM_X11) {
        info.api = NativeWindowApi::Xcb;
#ifdef __linux__
        auto display = glfwGetX11Display();
        info.xcb.connection = XGetXCBConnection(display);
#endif
        info.xcb.window = static_cast<uint32_t>(glfwGetX11Window(m_window));
      }
    }

    return info;
  }

  bool IsResized() const override { return m_isResized; };
  bool IsMinimized() const override { return m_width == 0 || m_height == 0; }
  void ResetResizeFlag() override { m_isResized = false; }
  void PollEvents() override { glfwPollEvents(); }
  bool ShouldClose() const override { return glfwWindowShouldClose(m_window); }
  void GetFrameBufferSize(uint32_t &outWidth,
                          uint32_t &outHeight) const override {
    outWidth = m_width;
    outHeight = m_height;
  }

  void WaitEvents() const override { glfwWaitEvents(); }

private:
  GLFWwindow *m_window{nullptr};
  bool m_isResized = false;
  uint32_t m_width = 0;
  uint32_t m_height = 0;

  void InternalCleanup() {
    if (m_window) {
      glfwDestroyWindow(m_window);
      m_window = nullptr;
    }
    glfwTerminate();
  }
};
} // namespace avalon::window

extern "C" AVALON_WINDOW_GLFW_API avalon::IPlugin *CreatePlugin() {
  return new avalon::window::GlfwWindow();
}

extern "C" AVALON_WINDOW_GLFW_API void DestroyPlugin(avalon::IPlugin *plugin) {
  if (plugin) {
    delete plugin;
  }
}
