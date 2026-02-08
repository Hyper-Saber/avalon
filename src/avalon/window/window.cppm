module;
#include <cstdint>

export module avalon.window;

import avalon.core;

export namespace avalon::window {

struct WindowProps {
  const char *title = "Avalon Engine";
  uint32_t width = 1280;
  uint32_t height = 720;
  bool resizable = true;
  bool vsync = false;
  WindowMode mode = WindowMode::Windowed;
  WindowBackend backend = WindowBackend::Auto;
};

class IWindow : public IPlugin {
public:
  virtual auto Initialize(const WindowProps &props) -> EStatusCode = 0;
  virtual auto GetNativeInfo() -> NativeWindowInfo const = 0;
  virtual bool IsMinimized() const = 0;
  virtual bool WasResized() const = 0;
  virtual void ResetResizeFlag() = 0;
  virtual void PollEvents() = 0;
  virtual bool ShouldClose() const = 0;
  virtual void GetFrameBufferSize(uint32_t &outWidth,
                                  uint32_t &outHeight) const = 0;
  virtual void WaitEvents() const = 0;
};
} // namespace avalon::window
