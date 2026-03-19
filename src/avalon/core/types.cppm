module;
#include <cstdint>

export module avalon.core:types;

export namespace avalon {

struct Color {
  float r = 0, g = 0, b = 0, a = 1;

  constexpr Color() = default;
  constexpr Color(float r, float g, float b, float a = 1.0f)
      : r(r), g(g), b(b), a(a) {}

  static constexpr Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
  static constexpr Color Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
  static constexpr Color Transparent() {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }

  static constexpr Color Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
  static constexpr Color Green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
  static constexpr Color Blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
  static constexpr Color Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
  static constexpr Color Cyan() { return {0.0f, 1.0f, 1.0f, 1.0f}; }
  static constexpr Color Magenta() { return {1.0f, 0.0f, 1.0f, 1.0f}; }

  static constexpr Color Gray() { return {0.5f, 0.5f, 0.5f, 1.0f}; }
  static constexpr Color DarkGray() {
    return {0.25f, 0.25f, 0.25f, 1.0f};
  }
  static constexpr Color LightGray() {
    return {0.75f, 0.75f, 0.75f, 1.0f};
  }

  static constexpr Color Lerp(const Color &a, const Color &b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t};
  }
};

} // namespace avalon
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
