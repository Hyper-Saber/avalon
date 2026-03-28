export module avalon.input:input_mapping;
import avalon.core;
import :types;

export namespace avalon::input {

struct InputMapping {
  HashMap<StringId, ActionBinding> actions;
  HashMap<StringId, TriggerBinding> triggers;
  HashMap<StringId, AxisBinding> axes;

  static InputMapping CreateDefaultFPS() {
    InputMapping map;

    map.actions["Jump"_id] = {
        .button = EGamepadButton::A,
        .key = EKeyCode::Backspace,
    };
    map.actions["Interact"_id] = {
        .button = EGamepadButton::X,
        .key = EKeyCode::E,
    };
    map.actions["Reload"_id] = {
        .button = EGamepadButton::Y,
        .key = EKeyCode::R,
    };
    map.actions["Crouch"_id] = {
        .button = EGamepadButton::B,
        .key = EKeyCode::LeftControl,
    };
    map.actions["Menu"_id] = {
        .button = EGamepadButton::Start,
        .key = EKeyCode::Escape,
    };

    map.triggers["Fire"_id] = {
        .axis = EGamepadAxis::RT,
        .mouseButton = EMouseButton::Left,
    };
    map.triggers["Aim"_id] = {
        .axis = EGamepadAxis::LT,
        .mouseButton = EMouseButton::Right,
    };

    map.axes["Move"_id] = {
        .source = EAxisSource::Gamepad,
        .isLeftAxis = true,
        .up = EKeyCode::W,
        .down = EKeyCode::S,
        .left = EKeyCode::A,
        .right = EKeyCode::D,
    };
    map.axes["Look"_id] = {
        .source = EAxisSource::Gamepad,
        .isLeftAxis = false,
    };

    return map;
  }
};

} // namespace avalon::input
