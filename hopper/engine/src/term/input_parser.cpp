#include <algorithm>
#include <charconv>
#include <optional>
#include <print>
#include <vector>

#include "term/input_parser.hpp"

using namespace core::input;

void InputParser::advance(uint32_t n) {
    cursor_ = std::min(cursor_ + n, (uint32_t)input_.size());
}

std::vector<Event> InputParser::parse() {
    cursor_ = 0;
    mode_ = Mode::Normal;
    input_events_.clear();

    while (cursor_ < input_.size()) {
        if (mode_ == Mode::Normal) {
            parse_char();
        } else { // Mode::Escape
            auto result = parse_escape_code();
            if (!result) {
                // Incomplete or malformed escape code, stop parsing to prevent errors.
                break;
            }
            mode_ = Mode::Normal;
        }
    }
    return input_events_;
}

void InputParser::parse_char() {
    char c = input_[cursor_];
    advance();
    if (c == '\x1b') {
        mode_ = Mode::Escape;
        return;
    }

    std::optional<KeyCode> key_code;
    switch (c) {
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
        input_events_.push_back(KeyEvent {key_code.value()});
    }
}

std::expected<void, InputParser::ParseError> InputParser::parse_escape_code() {
    if (cursor_ >= input_.size()) {
        return std::unexpected(ParseError::Incomplete);
    }

    // Only CSI sequences (starting with '[') are handled for now.
    if (input_[cursor_] != '[') {
        advance(); // Skip the character after ESC.
        return {};
    }
    advance(); // Skip '['.

    size_t seq_start = cursor_;
    char terminator = 0;
    while (cursor_ < input_.size()) {
        char c = input_[cursor_];
        // Valid terminating character in the range'@' (0x40)  through  '~' (0x7E)
        if (c >= 0x40 && c <= 0x7E) {
            terminator = c;
            break;
        }
        advance();
    }

    if (terminator == 0) {
        return std::unexpected(ParseError::Incomplete);
    }

    std::string_view seq(input_.data() + seq_start, cursor_ - seq_start);
    advance(); // Move past the terminator.

    // --- Sequence Parsing --- //
    auto parse_params = [](std::string_view sv) {
        std::vector<uint32_t> params;
        while (!sv.empty()) {
            size_t delimiter_pos = sv.find(';');
            std::string_view num_str = sv.substr(0, delimiter_pos);
            uint32_t val = 0;
            if (std::from_chars(num_str.data(), num_str.data() + num_str.size(), val).ec
                == std::errc())
            {
                params.push_back(val);
            }
            if (delimiter_pos == std::string_view::npos)
                break;
            sv = sv.substr(delimiter_pos + 1);
        }
        return params;
    };

    // SGR Mouse Event: <button;x;y
    if ((terminator == 'M' || terminator == 'm') && !seq.empty() && seq.starts_with('<')) {
        auto nums = parse_params(seq.substr(1));

        if (nums.size() == 3) {
            uint32_t cb = nums[0];
            MouseEvent::Action action;
            if (cb & 32) {
                action = MouseEvent::Action::Move;
            } else {
                action =
                    (terminator == 'M') ? MouseEvent::Action::Press : MouseEvent::Action::Release;
            }

            std::optional<MouseButton> btn;
            switch (cb & 3) {
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
                    break;
            }

            input_events_.push_back(MouseEvent {
                .action = action,
                .row = nums[2], // SGR is col, then row
                .col = nums[1],
                .button = btn,
                .shift = (bool)(cb & 4),
                .ctrl = (bool)(cb & 16),
                .alt = (bool)(cb & 8),
            });
        }
    } else {
        // Key press events  - Function keys, direction buttons etc.
        std::optional<KeyCode> key_code;
        bool shift = false, alt = false, ctrl = false;

        auto params = parse_params(seq);
        int modifier = 0;
        if (params.size() > 1) {
            modifier = params[1];
        }

        if (modifier > 0) {
            shift = (modifier - 1) & 1;
            alt = (modifier - 1) & 2;
            ctrl = (modifier - 1) & 4;
        }

        switch (terminator) {
            case 'A':
                key_code = KeyCode::Up;
                break;
            case 'B':
                key_code = KeyCode::Down;
                break;
            case 'C':
                key_code = KeyCode::Right;
                break;
            case 'D':
                key_code = KeyCode::Left;
                break;
            default:
                break;
        }

        if (key_code) {
            input_events_.push_back(KeyEvent {*key_code, shift, ctrl, alt});
        }
    }

    return {};
}
