#pragma once

enum class InputAction {
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Cancel,
    Start,
    Reload,
    Fire,
    None
};

enum class ControlScheme {
    SinglePlayer,
    DuelLeftPlayer,
    DuelRightPlayer
};

bool isCancelKey(int ch);
InputAction getInputAction(int ch, ControlScheme scheme = ControlScheme::SinglePlayer);
