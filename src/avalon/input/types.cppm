module;

export module avalon.input:types;
import avalon.window;

export namespace avalon::input {

enum class EAxisSource { Gamepad, Keyboard, Mouse };

struct ActionBinding {
  EGamepadButton button = EGamepadButton::Last;
  EKeyCode key = EKeyCode::Unknown;
  bool isMouse = false;
  EMouseButton mouseButton = EMouseButton::Left;
};

struct TriggerBinding {
  EGamepadAxis axis = EGamepadAxis::Last;
  EKeyCode key = EKeyCode::Unknown;
  EMouseButton mouseButton = EMouseButton::Left;
};

struct AxisBinding {
  EAxisSource source = EAxisSource::Gamepad;
  bool isLeftAxis = true;

  EKeyCode up = EKeyCode::Unknown;
  EKeyCode down = EKeyCode::Unknown;
  EKeyCode left = EKeyCode::Unknown;
  EKeyCode right = EKeyCode::Unknown;
};

} // namespace avalon::input
