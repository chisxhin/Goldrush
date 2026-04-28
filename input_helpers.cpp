#include "input_helpers.h"

#include <ncurses.h>

bool isCancelKey(int ch) {
    return ch == 27;
}

InputAction getInputAction(int ch, ControlScheme scheme) {
    if (isCancelKey(ch)) {
        return InputAction::Cancel;
    }

    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
        return InputAction::Confirm;
    }
    if (ch == ' ') {
        return InputAction::Fire;
    }
    if (ch == 'x' || ch == 'X') {
        return InputAction::Start;
    }
    if (ch == 'r' || ch == 'R') {
        return InputAction::Reload;
    }

    if (scheme == ControlScheme::DuelRightPlayer) {
        if (ch == KEY_UP) return InputAction::Up;
        if (ch == KEY_DOWN) return InputAction::Down;
        if (ch == KEY_LEFT) return InputAction::Left;
        if (ch == KEY_RIGHT) return InputAction::Right;
        return InputAction::None;
    }

    if (scheme == ControlScheme::DuelLeftPlayer) {
        if (ch == 'w' || ch == 'W') return InputAction::Up;
        if (ch == 's' || ch == 'S') return InputAction::Down;
        if (ch == 'a' || ch == 'A') return InputAction::Left;
        if (ch == 'd' || ch == 'D') return InputAction::Right;
        return InputAction::None;
    }

    if (ch == KEY_UP || ch == 'w' || ch == 'W') return InputAction::Up;
    if (ch == KEY_DOWN || ch == 's' || ch == 'S') return InputAction::Down;
    if (ch == KEY_LEFT || ch == 'a' || ch == 'A') return InputAction::Left;
    if (ch == KEY_RIGHT || ch == 'd' || ch == 'D') return InputAction::Right;
    return InputAction::None;
}
