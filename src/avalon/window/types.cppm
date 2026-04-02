module;
#include <array>
#include <cstddef>
#include <cstdint>

export module avalon.window:types;
import avalon.core;

export namespace avalon::window {

enum class WindowMode { Windowed, Fullscreen, Borderless };

enum class WindowBackend {
  Auto,
  Glfw,
  // Wayland, Win32
};

struct WindowProps {
  String title = "Avalon Engine";
  uint32_t width = 1280;
  uint32_t height = 720;
};

} // namespace avalon::window

export namespace avalon::input {

struct GamepadInfo {
  String name;
  int joystickId;
};

enum class ECursorMode { Normal, Hidden, Disabled };

enum class EKeyCode : uint16_t {
  Unknown = 0,
  A = 65,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  D0 = 48,
  D1,
  D2,
  D3,
  D4,
  D5,
  D6,
  D7,
  D8,
  D9,
  Escape = 256,
  Enter,
  Tab,
  Space,
  Insert,
  Delete,
  Right,
  Left,
  Down,
  Up,
  PageUp,
  PageDown,
  Home,
  End,
  CapsLock = 280,
  ScrollLock,
  NumLock,
  PrintScreen,
  Pause,
  F1 = 290,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  LeftShift = 340,
  LeftControl,
  LeftAlt,
  LeftSuper,
  RightShift,
  RightControl,
  RightAlt,
  RightSuper
};

enum class EMouseButton : int {
  Left = 0,
  Right = 1,
  Middle = 2,
  Button4 = 3,
  Button5 = 4,
  None = 5,
};

enum class EGamepadButton : int {
  A = 0,
  B,
  X,
  Y,
  LB,
  RB,
  Back,
  Start,
  Guide,
  LS,
  RS,
  Up,
  Right,
  Down,
  Left,
  None,
};

enum class EGamepadAxis : int {
  LeftX = 0,
  LeftY = 1,
  RightX = 2,
  RightY = 3,
  LT = 4,
  RT = 5,
  None = 6,
};

struct GamepadState {
  bool isConnected = false;
  std::array<bool, 15> buttons{};
  std::array<float, 6> axes{};

  bool IsPressed(EGamepadButton button) const {
    return buttons[static_cast<size_t>(button)];
  }

  float GetAxis(EGamepadAxis axis, float deadzone = 0.1f) const {
    float val = axes[static_cast<size_t>(axis)];

    if (axis == EGamepadAxis::LT || axis == EGamepadAxis::RT) {
      return (val < deadzone) ? 0.0f : val;
    }

    return (val > -deadzone && val < deadzone) ? 0.0f : val;
  }
};

struct KeyStateBuffer {
  std::array<uint64_t, 8> data{};

  void Set(EKeyCode key, bool pressed) {
    uint16_t k = static_cast<uint16_t>(key);
    if (k >= 512)
      return;
    uint64_t &chunk = data[k / 64];
    uint64_t mask = 1ULL << (k % 64);
    if (pressed)
      chunk |= mask;
    else
      chunk &= ~mask;
  }

  bool Get(EKeyCode key) const {
    uint16_t k = static_cast<uint16_t>(key);
    if (k >= 512)
      return false;
    return (data[k / 64] & (1ULL << (k % 64))) != 0;
  }
};

struct FrameInputSnapshot {
  KeyStateBuffer keys;
  std::array<bool, 8> mouseButtons{};
  double mouseX{0.0}, mouseY{0.0};
  double scrollX{0.0}, scrollY{0.0};

  std::array<GamepadState, 4> gamepads{};
  uint32_t activeGamepadCount{0};

  bool IsKeyPressed(EKeyCode key) const { return keys.Get(key); }

  bool IsMouseButtonPressed(EMouseButton button) const {
    return mouseButtons[static_cast<size_t>(button)];
  }

  String ToString() const {
    String res = "InputSnapshot:\n";

    res +=
        String::Format("  Mouse: ({:.2f}, {:.2f}) | Scroll: ({:.2f}, {:.2f})\n",
                       mouseX, mouseY, scrollX, scrollY);

    String mb = "  Buttons: [ ";
    for (size_t i = 0; i < mouseButtons.size(); ++i) {
      if (mouseButtons[i])
        mb += String::Format("{} ", i);
    }
    res += mb + "]\n";

    res += "  Keyboard: (Active Keys in buffer)\n";

    res += String::Format("  Gamepads: (Active: {})\n", activeGamepadCount);
    for (uint32_t i = 0; i < activeGamepadCount; ++i) {
      const auto &pad = gamepads[i];
      res += String::Format(
          "    [{}] LX: {:.2f}, LY: {:.2f} | RX: {:.2f}, RY: {:.2f} | LT: "
          "{:.2f}, RT: {:.2f}\n",
          i, pad.GetAxis(EGamepadAxis::LeftX), pad.GetAxis(EGamepadAxis::LeftY),
          pad.GetAxis(EGamepadAxis::RightX), pad.GetAxis(EGamepadAxis::RightY),
          pad.GetAxis(EGamepadAxis::LT), pad.GetAxis(EGamepadAxis::RT));
    }

    return res;
  }
};

} // namespace avalon::input
