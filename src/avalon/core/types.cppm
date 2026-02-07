module;
#include <cstdint>
export module avalon.core:types;

import :vfs;

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

enum class EFormat {
  R32G32B32A32_Float4,
  R32G32B32_Float3,
  R32G32_Float2,
  R32_Float,
};

struct RhiConfig {
  RenderBackend backend = RenderBackend::Auto;
  bool enableValidationLayer = true;
};
} // namespace avalon::rhi

export namespace avalon {
struct EngineContext {
  rhi::RenderBackend rhi;
  vfs::IVfs *pVfs;
};

struct EngineConfig {};

AVALON_CORE_API auto GetContext() -> EngineContext & {
  static EngineContext instance;
  return instance;
}
} // namespace avalon
