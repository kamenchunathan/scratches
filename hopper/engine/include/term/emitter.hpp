#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "color.hpp"
#include "common.hpp"

namespace term {

class TerminalEmitter {
public:
    explicit TerminalEmitter(std::vector<char>& buffer);

    void append(char c) {
        buffer_.push_back(c);
    }

    void append(std::string_view s);
    void append_uint8(std::uint8_t n);

    void append_utf8(char32_t codepoint) {
        update_styles();
        if (codepoint <= 0x7F) {
            append(static_cast<char>(codepoint));
        } else {
            append_utf8_slow(codepoint);
        }
    }

    void set_fg(core::ColorRGB8 color);
    void set_bg(core::ColorRGB8 color);

    void reset();

    void move_cursor(std::uint32_t row, std::uint32_t col);

    /**
     * @brief Appends the ANSI REP sequence (CSI Pn b) to repeat the last character.
     * @param count The number of times to repeat (Pn).
     */
    void repeat(std::uint32_t count);

private:
    std::vector<char>& buffer_;

    core::ColorRGB8 current_fg_ {0, 0, 0};
    core::ColorRGB8 current_bg_ {0, 0, 0};
    bool fg_dirty_ = true;
    bool bg_dirty_ = true;
    bool is_reset_ = true;

    void update_styles();
    void append_utf8_slow(char32_t codepoint);
};

} // namespace term
