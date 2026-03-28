module;
#include <cstdint>

export module avalon.window;
export import :types;
import avalon.core;

using namespace avalon::input;

export namespace avalon::window {

class IWindow : public IPlugin {
public:
  virtual void SetTitle(StringView title) = 0;
  virtual auto Initialize(const WindowProps &props) -> EStatusCode = 0;
  virtual auto GetNativeInfo() -> NativeWindowInfo const = 0;
  virtual bool IsMinimized() const = 0;
  virtual bool IsResized() const = 0;
  virtual void ResetResizeFlag() = 0;
  virtual void PollEvents() = 0;
  virtual bool ShouldClose() const = 0;
  virtual void GetFrameBufferSize(uint32_t &outWidth,
                                  uint32_t &outHeight) const = 0;
  virtual void WaitEvents() const = 0;

  virtual void SetCursorMode(input::ECursorMode mode) = 0;

  virtual bool IsGamepadPresent(int joystickId) const = 0;
  virtual auto GetGamepadInfos() -> Span<const input::GamepadInfo> = 0;
  virtual auto GetInputSnapshot() -> input::FrameInputSnapshot & = 0;
};
} // namespace avalon::window
