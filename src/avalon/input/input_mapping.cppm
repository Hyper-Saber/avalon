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
        .key = EKeyCode::Space,
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

  static InputMapping CreateDefaultDrone() {
    InputMapping map;

    map.actions["Menu"_id] = {
        .button = EGamepadButton::Start,
        .key = EKeyCode::Escape,
    };

    map.triggers["Ascend"_id] = {
        .axis = EGamepadAxis::RT,
        .key = EKeyCode::Space,
    };
    map.triggers["Descend"_id] = {
        .axis = EGamepadAxis::LT,
        .key = EKeyCode::LeftShift,
    };

    map.actions["RollLeft"_id] = {
        .button = EGamepadButton::LB,
        .key = EKeyCode::Q,
    };
    map.actions["RollRight"_id] = {
        .button = EGamepadButton::RB,
        .key = EKeyCode::E,
    };

    map.axes["FlightMove"_id] = {
        .source = EAxisSource::Gamepad,
        .isLeftAxis = true,
        .up = EKeyCode::W,
        .down = EKeyCode::S,
        .left = EKeyCode::A,
        .right = EKeyCode::D,
    };

    map.axes["CameraLook"_id] = {
        .source = EAxisSource::Gamepad,
        .isLeftAxis = false,
    };

    return map;
  }
};

} // namespace avalon::input
