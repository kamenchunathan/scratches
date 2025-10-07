#pragma once

#include <format>
#include <optional>
#include <variant>

namespace core::input {

enum class KeyCode {
    Unknown,
    A,
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
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Enter,
    Escape,
    Backspace,
    Tab,
    Space,
    Up,
    Down,
    Left,
    Right,
    F1,
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
    PageUp,
    PageDown,
    Home,
    End,
    Insert,
    Delete,
};

struct KeyEvent {
    enum class Action {
        Press,
        Release,
    };

    KeyCode code;
    Action action;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
};

enum class MouseButton {
    Left,
    Right,
    Middle,
};

struct MouseEvent {
    enum class Action {
        Press,
        Release,
        Move,
    };

    Action action;
    std::uint32_t row, col;
    std::optional<MouseButton> button;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
};

struct ResizeEvent {
    int width, height;
};

using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent>;

} // namespace core::input

template<>
struct std::formatter<core::input::KeyCode>: std::formatter<std::string_view> {
    auto format(core::input::KeyCode c, std::format_context& ctx) const {
        using core::input::KeyCode;
        std::string_view s;
        switch (c) {
            case KeyCode::Unknown:
                s = "Unknown";
                break;
            case KeyCode::A:
                s = "A";
                break;
            case KeyCode::B:
                s = "B";
                break;
            case KeyCode::C:
                s = "C";
                break;
            case KeyCode::D:
                s = "D";
                break;
            case KeyCode::E:
                s = "E";
                break;
            case KeyCode::F:
                s = "F";
                break;
            case KeyCode::G:
                s = "G";
                break;
            case KeyCode::H:
                s = "H";
                break;
            case KeyCode::I:
                s = "I";
                break;
            case KeyCode::J:
                s = "J";
                break;
            case KeyCode::K:
                s = "K";
                break;
            case KeyCode::L:
                s = "L";
                break;
            case KeyCode::M:
                s = "M";
                break;
            case KeyCode::N:
                s = "N";
                break;
            case KeyCode::O:
                s = "O";
                break;
            case KeyCode::P:
                s = "P";
                break;
            case KeyCode::Q:
                s = "Q";
                break;
            case KeyCode::R:
                s = "R";
                break;
            case KeyCode::S:
                s = "S";
                break;
            case KeyCode::T:
                s = "T";
                break;
            case KeyCode::U:
                s = "U";
                break;
            case KeyCode::V:
                s = "V";
                break;
            case KeyCode::W:
                s = "W";
                break;
            case KeyCode::X:
                s = "X";
                break;
            case KeyCode::Y:
                s = "Y";
                break;
            case KeyCode::Z:
                s = "Z";
                break;
            case KeyCode::Num0:
                s = "0";
                break;
            case KeyCode::Num1:
                s = "1";
                break;
            case KeyCode::Num2:
                s = "2";
                break;
            case KeyCode::Num3:
                s = "3";
                break;
            case KeyCode::Num4:
                s = "4";
                break;
            case KeyCode::Num5:
                s = "5";
                break;
            case KeyCode::Num6:
                s = "6";
                break;
            case KeyCode::Num7:
                s = "7";
                break;
            case KeyCode::Num8:
                s = "8";
                break;
            case KeyCode::Num9:
                s = "9";
                break;
            case KeyCode::Enter:
                s = "Enter";
                break;
            case KeyCode::Escape:
                s = "Escape";
                break;
            case KeyCode::Backspace:
                s = "Backspace";
                break;
            case KeyCode::Tab:
                s = "Tab";
                break;
            case KeyCode::Space:
                s = "Space";
                break;
            case KeyCode::Up:
                s = "Up";
                break;
            case KeyCode::Down:
                s = "Down";
                break;
            case KeyCode::Left:
                s = "Left";
                break;
            case KeyCode::Right:
                s = "Right";
                break;
            case KeyCode::F1:
                s = "F1";
                break;
            case KeyCode::F2:
                s = "F2";
                break;
            case KeyCode::F3:
                s = "F3";
                break;
            case KeyCode::F4:
                s = "F4";
                break;
            case KeyCode::F5:
                s = "F5";
                break;
            case KeyCode::F6:
                s = "F6";
                break;
            case KeyCode::F7:
                s = "F7";
                break;
            case KeyCode::F8:
                s = "F8";
                break;
            case KeyCode::F9:
                s = "F9";
                break;
            case KeyCode::F10:
                s = "F10";
                break;
            case KeyCode::F11:
                s = "F11";
                break;
            case KeyCode::F12:
                s = "F12";
                break;
            case KeyCode::PageUp:
                s = "PageUp";
                break;
            case KeyCode::PageDown:
                s = "PageDown";
                break;
            case KeyCode::Home:
                s = "Home";
                break;
            case KeyCode::End:
                s = "End";
                break;
            case KeyCode::Insert:
                s = "Insert";
                break;
            case KeyCode::Delete:
                s = "Delete";
                break;
        }
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

template<>
struct std::formatter<core::input::KeyEvent::Action>: std::formatter<std::string_view> {
    auto format(core::input::KeyEvent::Action c, std::format_context& ctx) const {
        switch (c) {
            case core::input::KeyEvent::Action::Press:
                return std::formatter<std::string_view>::format("Press", ctx);
            case core::input::KeyEvent::Action::Release:
                return std::formatter<std::string_view>::format("Release", ctx);
        };
    };
};

template<>
struct std::formatter<core::input::KeyEvent>: std::formatter<std::string_view> {
    auto format(core::input::KeyEvent c, std::format_context& ctx) const {
        return std::format_to(
            ctx.out(),
            "KeyEvent {{ code: {}, action: {}{}{}{} }}",
            c.code,
            c.action,
            c.shift ? ", shift" : "",
            c.ctrl ? ", ctrl" : "",
            c.alt ? ", alt" : ""
        );
    }
};

template<>
struct std::formatter<core::input::MouseEvent::Action>: std::formatter<std::string_view> {
    auto format(core::input::MouseEvent::Action c, std::format_context& ctx) const {
        switch (c) {
            case core::input::MouseEvent::Action::Press:
                return std::formatter<std::string_view>::format("Press", ctx);
            case core::input::MouseEvent::Action::Release:
                return std::formatter<std::string_view>::format("Release", ctx);
            case core::input::MouseEvent::Action::Move:
                return std::formatter<std::string_view>::format("Move", ctx);
        };
    };
};

template<>
struct std::formatter<core::input::MouseButton>: std::formatter<std::string_view> {
    auto format(core::input::MouseButton c, std::format_context& ctx) const {
        switch (c) {
            case core::input::MouseButton::Left:
                return std::formatter<std::string_view>::format("Left", ctx);
            case core::input::MouseButton::Middle:
                return std::formatter<std::string_view>::format("Middle", ctx);
            case core::input::MouseButton::Right:
                return std::formatter<std::string_view>::format("Right", ctx);
        };
        return ctx.out();
    };
};

template<>
struct std::formatter<core::input::MouseEvent>: std::formatter<std::string_view> {
    auto format(core::input::MouseEvent e, std::format_context& ctx) const {
        auto base_format =
            std::format("MouseEvent {{ action: {}, row: {}, col: {}", e.action, e.row, e.col);

        if (e.button) {
            base_format += std::format(", button: {}", *e.button);
        }

        if (e.shift)
            base_format += ", shift";
        if (e.ctrl)
            base_format += ", ctrl";
        if (e.alt)
            base_format += ", alt";

        base_format += " }}";

        return std::format_to(ctx.out(), "{}", base_format);
    }
};

template<>
struct std::formatter<core::input::ResizeEvent>: std::formatter<std::string_view> {
    auto format(core::input::ResizeEvent e, std::format_context& ctx) const {
        return std::format_to(
            ctx.out(),
            "ResizeEvent {{ width: {}, height: {} }}",
            e.width,
            e.height
        );
    }
};

template<>
struct std::formatter<core::input::Event>: std::formatter<std::string_view> {
    auto format(const core::input::Event& e, std::format_context& ctx) const {
        return std::visit(
            [&](const auto& event) { return std::format_to(ctx.out(), "{}", event); },
            e
        );
    }
};
