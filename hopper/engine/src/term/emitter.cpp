#include "term/emitter.hpp"

namespace term {

TerminalEmitter::TerminalEmitter(std::vector<char>& buffer): buffer_(buffer) {}

void TerminalEmitter::append(std::string_view s) {
    buffer_.insert(buffer_.end(), s.begin(), s.end());
}

void TerminalEmitter::append_uint8(std::uint8_t n) {
    if (n >= 100) {
        append(static_cast<char>('0' + (n / 100)));
        n %= 100;
        append(static_cast<char>('0' + (n / 10)));
        append(static_cast<char>('0' + (n % 10)));
    } else if (n >= 10) {
        append(static_cast<char>('0' + (n / 10)));
        append(static_cast<char>('0' + (n % 10)));
    } else {
        append(static_cast<char>('0' + n));
    }
}

void TerminalEmitter::append_utf8_slow(char32_t cp) {
    if (cp <= 0x7FF) {
        append(static_cast<char>(0xC0 | (cp >> 6)));
        append(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        append(static_cast<char>(0xE0 | (cp >> 12)));
        append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        append(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        append(static_cast<char>(0xF0 | (cp >> 18)));
        append(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        append(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

void TerminalEmitter::set_fg(core::ColorRGB8 color) {
    if (is_reset_ || current_fg_ != color) {
        current_fg_ = color;
        fg_dirty_   = true;
        is_reset_   = false;
    }
}

void TerminalEmitter::set_bg(core::ColorRGB8 color) {
    if (is_reset_ || current_bg_ != color) {
        current_bg_ = color;
        bg_dirty_   = true;
        is_reset_   = false;
    }
}

void TerminalEmitter::reset() {
    if (!is_reset_) {
        append("\x1b[0m");
        is_reset_ = true;
        fg_dirty_ = false;
        bg_dirty_ = false;
    }
}

void TerminalEmitter::move_cursor(std::uint32_t row, std::uint32_t col) {
    append("\x1b[");
    // Using a simple loop for row/col as they are less frequent than color changes
    auto append_uint = [this](std::uint32_t n) {
        if (n == 0) {
            append('0');
            return;
        }
        char buf[12];
        int i = 0;
        while (n > 0) {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
        while (i > 0) {
            append(buf[--i]);
        }
    };
    append_uint(row);
    append(';');
    append_uint(col);
    append('H');
}

void TerminalEmitter::repeat(std::uint32_t count) {
    if (count <= 1)
        return; // 1 means no repeat needed (already wrote one)
    append("\x1b[");
    // REP sequence count is the number of times to repeat the PREVIOUS character.
    // If we want 5 characters total, we write 1 and then REP 4.

    auto append_uint = [this](std::uint32_t n) {
        if (n == 0) {
            append('0');
            return;
        }
        char buf[12];
        int i = 0;
        while (n > 0) {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
        while (i > 0) {
            append(buf[--i]);
        }
    };
    append_uint(count - 1);
    append('b');
}

void TerminalEmitter::update_styles() {
    if (!fg_dirty_ && !bg_dirty_)
        return;

    append("\x1b[");
    if (fg_dirty_) {
        append("38;2;");
        append_uint8(current_fg_.r);
        append(';');
        append_uint8(current_fg_.g);
        append(';');
        append_uint8(current_fg_.b);
        fg_dirty_ = false;
        if (bg_dirty_)
            append(';');
    }
    if (bg_dirty_) {
        append("48;2;");
        append_uint8(current_bg_.r);
        append(';');
        append_uint8(current_bg_.g);
        append(';');
        append_uint8(current_bg_.b);
        bg_dirty_ = false;
    }
    append('m');
}

} // namespace term
