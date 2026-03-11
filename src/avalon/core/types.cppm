module;
#include <cstdint>

export module avalon.core:types;

export namespace avalon::window {

enum class WindowMode { Windowed, Fullscreen, Borderless };

enum class WindowBackend {
  Auto,
  Glfw,
  // Wayland, Win32
};

enum class NativeWindowApi {
  Wayland,
  Xcb,
  Win32,
};

struct NativeWindowInfo {
  NativeWindowApi api;

  union {
    struct {
      void *display;
      void *surface;
    } wayland;
    struct {
      void *connection;
      uint32_t window;
    } xcb;
    struct {
      void *hwnd;
      void *hinstance;
    };
  };
};
} // namespace avalon::window

export namespace avalon::rhi {
enum class RenderBackend {
  Auto,
  Vulkan,
  // D3D12, Metal
};

struct RhiConfig {
  RenderBackend backend = RenderBackend::Auto;
  bool enableValidationLayer = true;
};
} // namespace avalon::rhi
