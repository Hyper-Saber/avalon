module;
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <utility>

#ifdef __linux__
#include <X11/Xlib-xcb.h>
#endif

export module avalon.window.glfw;
import avalon.core;
import avalon.window;

using namespace avalon::input;

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
#ifdef WAYLAND_CLIENT_H
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "avalon");
#endif // WAYLAND_CLIENT_H
    m_window = glfwCreateWindow(props.width, props.height,
                                props.title.GetData(), nullptr, nullptr);

    if (!m_window) {
      avalon::Error("[GlfwWindow]: Failed to create GLFW window!");
      return EStatusCode::WindowError;
    }

    glfwSetWindowAspectRatio(m_window, props.width, props.height);

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

    glfwSetKeyCallback(m_window, [](GLFWwindow *window, int key, int scancode,
                                    int action, int mods) {
      auto self = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
      if (key >= 0 && key < 512) {
        bool pressed = (action != GLFW_RELEASE);
        self->m_snapshot.keys.Set(static_cast<input::EKeyCode>(key), pressed);
      }
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow *window, int button,
                                            int action, int mods) {
      auto self = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
      if (button >= 0 && button < 8) {
        self->m_snapshot.mouseButtons[button] = (action != GLFW_RELEASE);
      }
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow *window, double xoffset,
                                       double yoffset) {
      auto self = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
      self->m_snapshot.scrollX += xoffset;
      self->m_snapshot.scrollY += yoffset;
    });

    for (int joystickId = GLFW_JOYSTICK_1; joystickId <= GLFW_JOYSTICK_16;
         ++joystickId) {
      if (glfwJoystickPresent(joystickId)) {
        if (glfwJoystickIsGamepad(joystickId)) {
          m_connectedGamepads.PushBack({.name = glfwGetGamepadName(joystickId),
                                        .joystickId = joystickId});
        }
      }
    }

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

  void PollEvents() override {
    m_snapshot.scrollX = 0.0;
    m_snapshot.scrollY = 0.0;

    glfwPollEvents();

    glfwGetCursorPos(m_window, &m_snapshot.mouseX, &m_snapshot.mouseY);

    m_snapshot.activeGamepadCount = 0;
    for (size_t i = 0; i < m_connectedGamepads.GetSize() && i < 4; ++i) {
      int jid = m_connectedGamepads[i].joystickId;
      if (GetGamepadState(jid, m_snapshot.gamepads[i])) {
        m_snapshot.activeGamepadCount++;
      }
    }
  }

  bool ShouldClose() const override { return glfwWindowShouldClose(m_window); }
  void GetFrameBufferSize(uint32_t &outWidth,
                          uint32_t &outHeight) const override {
    outWidth = m_width;
    outHeight = m_height;
  }

  void WaitEvents() const override { glfwWaitEvents(); }

  void SetCursorMode(ECursorMode mode) override {
    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode) {
    case ECursorMode::Normal:
      glfwMode = GLFW_CURSOR_NORMAL;
      break;
    case ECursorMode::Hidden:
      glfwMode = GLFW_CURSOR_HIDDEN;
      break;
    case ECursorMode::Disabled:
      glfwMode = GLFW_CURSOR_DISABLED;
      break;
    }
    glfwSetInputMode(m_window, GLFW_CURSOR, glfwMode);
  }

  bool IsGamepadPresent(int joystickId) const override {
    return glfwJoystickPresent(joystickId) && glfwJoystickIsGamepad(joystickId);
  }

  auto GetGamepadInfos() -> Span<const GamepadInfo> override {
    return m_connectedGamepads;
  }

  auto GetInputSnapshot() -> input::FrameInputSnapshot & override {
    return m_snapshot;
  }

private:
  bool GetGamepadState(int joystickId, GamepadState &outState) const {
    GLFWgamepadstate glfwState;

    if (glfwGetGamepadState(joystickId, &glfwState)) {
      outState.isConnected = true;

      for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i) {
        outState.buttons[i] = (glfwState.buttons[i] == GLFW_PRESS);
      }

      for (int i = 0; i <= GLFW_GAMEPAD_AXIS_RIGHT_Y; ++i) {
        outState.axes[i] = glfwState.axes[i];
      }

      auto normalizeTrigger = [](float v) { return (v + 1.0f) * 0.5f; };

      outState.axes[static_cast<size_t>(EGamepadAxis::LT)] =
          normalizeTrigger(glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);

      outState.axes[static_cast<size_t>(EGamepadAxis::RT)] =
          normalizeTrigger(glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);

      return true;
    }

    outState.isConnected = false;
    return false;
  }

  void InternalCleanup() {
    if (m_window) {
      glfwDestroyWindow(m_window);
      m_window = nullptr;
    }
    glfwTerminate();
  }

  GLFWwindow *m_window{nullptr};
  bool m_isResized = false;
  uint32_t m_width = 0;
  uint32_t m_height = 0;

  FrameInputSnapshot m_snapshot;
  Array<GamepadInfo> m_connectedGamepads;
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
