module;
#include <cstdint>
#include <utility>
export module avalon.input:input_manager;
import :types;
import :input_mapping;
import avalon.core;
import avalon.window;

export namespace avalon::input {

class InputManager {
public:
  void LoadMapping(InputMapping &&mapping) {
    m_actions.Swap(mapping.actions);
    m_triggers.Swap(mapping.triggers);
    m_axes.Swap(mapping.axes);
  }

  void BindAction(StringId name, EGamepadButton button,
                  EKeyCode key = EKeyCode::Unknown) {
    m_actions[name] = {
        .button = button,
        .key = key,
        .mouseButton = EMouseButton::Last,
    };
  }

  void BindAction(StringId name, EGamepadButton button,
                  EMouseButton key = EMouseButton::Last) {
    m_actions[name] = {
        .button = button,
        .key = EKeyCode::Unknown,
        .mouseButton = key,
    };
  }

  void BindTrigger(StringId name, EGamepadAxis axis,
                   EKeyCode key = EKeyCode::Unknown) {
    m_triggers[name] = {
        .axis = axis,
        .key = key,
        .mouseButton = EMouseButton::Last,
    };
  }

  void BindTrigger(StringId name, EGamepadAxis axis,
                   EMouseButton mouseButton = EMouseButton::Last) {
    m_triggers[name] = {
        .axis = axis,
        .key = EKeyCode::Unknown,
        .mouseButton = mouseButton,
    };
  }

  void SetAxisSource(StringId name, EAxisSource source) {
    m_axes[name].source = source;
  }

  void BindAxis(StringId name, bool isLeft) {
    m_axes[name] = {.source = EAxisSource::Gamepad, .isLeftAxis = isLeft};
  }

  void BindCustomAxis(StringId name, EKeyCode u, EKeyCode d, EKeyCode l,
                      EKeyCode r) {
    auto &b = m_axes[name];
    b.source = EAxisSource::Keyboard;
    b.up = u;
    b.down = d;
    b.left = l;
    b.right = r;
  }

  void Update(const FrameInputSnapshot &latestSnapshot) {
    m_previous = m_current;
    m_current = latestSnapshot;
  }

  bool IsActionHolding(StringId name, uint32_t playerIndex = 0) const {
    auto *binding = m_actions.Get(name);
    if (!binding)
      return false;

    if (playerIndex < m_current.activeGamepadCount &&
        binding->button != EGamepadButton::Last) {
      if (m_current.gamepads[playerIndex].IsPressed(binding->button))
        return true;
    }
    if (binding->key != EKeyCode::Unknown)
      return m_current.IsKeyPressed(binding->key);

    return binding->mouseButton != EMouseButton::Last &&
           m_current.IsMouseButtonPressed(binding->mouseButton);
  }

  bool IsActionJustPressed(StringId name, uint32_t playerIndex = 0) const {
    auto *binding = m_actions.Get(name);
    if (!binding)
      return false;

    bool curr = IsActionHolding(name, playerIndex);
    bool prev = false;
    if (playerIndex < m_previous.activeGamepadCount &&
        binding->button != EGamepadButton::Last) {
      if (m_previous.gamepads[playerIndex].IsPressed(binding->button))
        prev = true;
    }
    if (!prev) {
      if (binding->key != EKeyCode::Unknown) {
        prev = m_previous.IsKeyPressed(binding->key);
      } else {
        prev = binding->mouseButton != EMouseButton::Last &&
               m_previous.IsMouseButtonPressed(binding->mouseButton);
      }
    }
    return curr && !prev;
  }

  float GetTriggerValue(StringId name, uint32_t playerIndex = 0) const {
    auto *binding = m_triggers.Get(name);
    if (!binding)
      return 0.0f;

    if (playerIndex < m_current.activeGamepadCount &&
        binding->axis != EGamepadAxis::Last) {
      float value = m_current.gamepads[playerIndex].GetAxis(binding->axis);
      if (Abs(value) > 0.0f)
        return value;
    }

    if (binding->key != EKeyCode::Unknown) {
      return m_current.IsKeyPressed(binding->key) ? 1.0f : 0.0f;
    }

    return binding->mouseButton != EMouseButton::Last &&
                   m_current.IsMouseButtonPressed(binding->mouseButton)
               ? 1.0f
               : 0.0f;
  }

  Vec2 GetAxisValue(StringId name, uint32_t playerIndex = 0) const {
    auto *binding = m_axes.Get(name);
    if (!binding)
      return {0.0f, 0.0f};

    switch (binding->source) {
    case EAxisSource::Gamepad: {
      if (playerIndex < m_current.activeGamepadCount) {
        auto xType =
            binding->isLeftAxis ? EGamepadAxis::LeftX : EGamepadAxis::RightX;
        auto yType =
            binding->isLeftAxis ? EGamepadAxis::LeftY : EGamepadAxis::RightY;
        Vec2 val = {m_current.gamepads[playerIndex].GetAxis(xType),
                    m_current.gamepads[playerIndex].GetAxis(yType)};

        if (Abs(val.x) > 0.0f || Abs(val.y) > 0.0f)
          return val;
      }
      return {0.0f, 0.0f};
    }

    case EAxisSource::Keyboard: {
      return GetCustomKeyboardAxis(binding->up, binding->down, binding->left,
                                   binding->right);
    }

    case EAxisSource::Mouse: {
      float m_sensitivity = 0.05f;
      return {static_cast<float>(m_current.mouseX - m_previous.mouseX) *
                  m_sensitivity,
              static_cast<float>(m_current.mouseY - m_previous.mouseY) *
                  m_sensitivity};
    }

    default:
      return {0.0f, 0.0f};
    }
  }

private:
  Vec2 GetCustomKeyboardAxis(EKeyCode u, EKeyCode d, EKeyCode l,
                             EKeyCode r) const {
    Vec2 axis{0.0f, 0.0f};
    if (m_current.IsKeyPressed(u))
      axis.y += 1.0f;
    if (m_current.IsKeyPressed(d))
      axis.y -= 1.0f;
    if (m_current.IsKeyPressed(r))
      axis.x += 1.0f;
    if (m_current.IsKeyPressed(l))
      axis.x -= 1.0f;
    return Normalize(axis);
  }

private:
  FrameInputSnapshot m_current;
  FrameInputSnapshot m_previous;
  HashMap<StringId, ActionBinding> m_actions;
  HashMap<StringId, TriggerBinding> m_triggers;
  HashMap<StringId, AxisBinding> m_axes;

  float m_mouseSensitivity = 0.05f;
};

} // namespace avalon::input
