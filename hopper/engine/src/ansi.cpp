#include <optional>
#include <print>

#include "ansi.hpp"
#include "input.hpp"

namespace ansi {

using namespace core::input;

std::vector<Event> InputParser::parse() {
    // TODO: UTF-8
    // Assumes cursor_ and mode_ are member variables of InputParser.
    cursor_ = 0;
    mode_ = Mode::Normal;

    while (cursor_ < input_.size()) {
        switch (mode_) {
            case Mode::Normal:
                parse_key_event();
                break;

            case Mode::Escape:
                parse_escape_code();
                mode_ = Mode::Normal;
                break;
        }
    }

    return input_events_;
}

void InputParser::parse_key_event() {
    char c = input_[cursor_++];
    std::optional<KeyCode> key_code;

    switch (c) {
        case '\x1b':
            // Beginning of an escape code. Change mode and wait for next characters.
            mode_ = Mode::Escape;
            break;
        case 'a':
            key_code = KeyCode::A;
            break;
        case 'b':
            key_code = KeyCode::B;
            break;
        case 'c':
            key_code = KeyCode::C;
            break;
        case 'd':
            key_code = KeyCode::D;
            break;
        case 'e':
            key_code = KeyCode::E;
            break;
        case 'f':
            key_code = KeyCode::F;
            break;
        case 'g':
            key_code = KeyCode::G;
            break;
        case 'h':
            key_code = KeyCode::H;
            break;
        case 'i':
            key_code = KeyCode::I;
            break;
        case 'j':
            key_code = KeyCode::J;
            break;
        case 'k':
            key_code = KeyCode::K;
            break;
        case 'l':
            key_code = KeyCode::L;
            break;
        case 'm':
            key_code = KeyCode::M;
            break;
        case 'n':
            key_code = KeyCode::N;
            break;
        case 'o':
            key_code = KeyCode::O;
            break;
        case 'p':
            key_code = KeyCode::P;
            break;
        case 'q':
            key_code = KeyCode::Q;
            break;
        case 'r':
            key_code = KeyCode::R;
            break;
        case 's':
            key_code = KeyCode::S;
            break;
        case 't':
            key_code = KeyCode::T;
            break;
        case 'u':
            key_code = KeyCode::U;
            break;
        case 'v':
            key_code = KeyCode::V;
            break;
        case 'w':
            key_code = KeyCode::W;
            break;
        case 'x':
            key_code = KeyCode::X;
            break;
        case 'y':
            key_code = KeyCode::Y;
            break;
        case 'z':
            key_code = KeyCode::Z;
            break;
        case '0':
            key_code = KeyCode::Num0;
            break;
        case '1':
            key_code = KeyCode::Num1;
            break;
        case '2':
            key_code = KeyCode::Num2;
            break;
        case '3':
            key_code = KeyCode::Num3;
            break;
        case '4':
            key_code = KeyCode::Num4;
            break;
        case '5':
            key_code = KeyCode::Num5;
            break;
        case '6':
            key_code = KeyCode::Num6;
            break;
        case '7':
            key_code = KeyCode::Num7;
            break;
        case '8':
            key_code = KeyCode::Num8;
            break;
        case '9':
            key_code = KeyCode::Num9;
            break;
        case ' ':
            key_code = KeyCode::Space;
            break;
        case '\n':
            key_code = KeyCode::Enter;
            break;
        case '\t':
            key_code = KeyCode::Tab;
            break;
        case '\b':
            key_code = KeyCode::Backspace;
            break;
        default:
            key_code = KeyCode::Unknown;
            break;
    }

    if (key_code) {
        input_events_.push_back(KeyEvent {key_code.value(), KeyEvent::Action::Press});
    }
}

void InputParser::parse_escape_code() {
    // Format: CSI < Cb ; Cx ; Cy (M or m)
    std::uint32_t x, y;
    MouseEvent::Action action;
    std::optional<MouseButton> btn;

    if (!try_parse_literal('['))
        return;
    if (!try_parse_literal('<'))
        return;

    auto opt = parse_int();
    if (!opt)
        return;
    switch (opt.value()) {
        case 0:
            btn = MouseButton::Left;
            break;

        case 1:
            btn = MouseButton::Middle;
            break;

        case 2:
            btn = MouseButton::Right;
            break;

        default:
            btn = std::nullopt;
    }

    if (!try_parse_literal(';'))
        return;

    opt = parse_int();
    if (!opt)
        return;
    x = opt.value();

    if (!try_parse_literal(';'))
        return;

    opt = parse_int();
    if (!opt)
        return;
    y = opt.value();

    if (try_parse_literal('M')) {
        action = MouseEvent::Action::Press;
    } else if (try_parse_literal('m')) {
        action = MouseEvent::Action::Release;
    } else {
        return;
    }
    mode_ = Mode::Normal;
    input_events_.push_back(MouseEvent {action, x, y, btn});
}

bool InputParser::try_parse_literal(char c) {
    if (input_[cursor_] == c) {
        cursor_++;
        return true;
    }

    return false;
}

std::optional<std::uint32_t> InputParser::parse_int() {
    std::uint32_t aggregate = 0;
    bool first_digit_parsed = false;

    while (cursor_ < input_.size()) {
        char c = input_[cursor_];
        if (c >= '0' && c <= '9') {
            first_digit_parsed = true;
            aggregate = aggregate * 10 + (c - '0');
            cursor_++;
        } else {
            break;
        }
    }

    if (!first_digit_parsed) {
        return std::nullopt;
    }

    return aggregate;
}

} // namespace ansi
