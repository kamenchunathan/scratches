#pragma once

#include <cstdint>
#include <expected>
#include <iostream>

#include "color.hpp"

namespace ansi {

inline void reset(std::ostream& os) {
    os << "\x1b[0m";
}

/* MouseInputMode
 * Enumerates the different xterm mouse tracking modes.
 * TODO: Encode tracking separately from encoding
 */
enum class MouseInputMode : std::int32_t {

    /* X10 Compatibility Mode
     * Sends an escape sequence only on button press,
     * encoding the mouse position and the button pressed.
     * Sequence format: CSI M Cb Cx Cy
     */
    X10 = 9,

    /* Normal Tracking (VT200) Mode
     * Sends escape sequences on both button press and release,
     * including modifier key (Shift, Meta, Control) states.
     * Sequence format: CSI M Cb Cx Cy
     */
    VT200 = 1000,

    /* Mouse Highlight Tracking Mode
     * Notifies the program of a button press, allows highlight region selection,
     * and sends release coordinates when the button is released.
     * Requires a cooperating program to respond with highlight parameters.
     */
    VT200Highlight = 1001,

    /* Button-Event Tracking Mode
     * Reports both press/release and motion events when a mouse button is held down.
     * Motion is reported only when the pointer moves to a different cell.
     */
    BtnEvent = 1002,

    /* Any-Event Tracking Mode
     * Reports motion events for all mouse movements, even when no button is pressed.
     * This mode produces a high volume of input and should be used with care.
     */
    AnyEvent = 1003,

};

/* Encoding of mouse coordinates in the escape sequences sent to the program.
 * Encoding modes can be combined with any tracking mode.
 */
enum class MouseEncodingMode : std::int32_t {

    /* Default (no extended encoding)
     * Uses 8-bit single-character coordinate encoding.
     * Limited to ~223x223 coordinate space.
     */
    Default = 0,

    /* UTF-8 Extended Mode
     * Expands coordinate range beyond 223 using UTF-8.
     * Legacy; replaced by SGR.
     */
    UTF8_Extended = 1005,

    /* SGR Extended Mode
     * Uses printable ASCII numeric encoding:
     * Format: CSI < Cb ; Cx ; Cy (M or m)
     * Supports large coordinates and simpler parsing.
     */
    SGR_Extended = 1006,

    /* urxvt Extended Mode
     * Alternative format used by rxvt-unicode:
     * Format: CSI Cb ; Cx ; Cy M
     */
    URxvt_Extended = 1015
};

/* Enables or disables xterm mouse tracking for the given mode.
 */
inline void mouse_input_tracking(
    std::ostream& os,
    bool enable,
    MouseInputMode mode,
    MouseEncodingMode encoding
) {
    os << "\x1b[?" << static_cast<std::int32_t>(mode) << (enable ? "h" : "l");

    if (encoding != MouseEncodingMode::Default) {
        os << "\x1b[?" << static_cast<std::int32_t>(encoding) << (enable ? "h" : "l");
    }
}

namespace fg {
    inline void rgb(std::ostream& os, uint8_t r, uint8_t g, uint8_t b) {
        os << "\x1b[38;2;" << static_cast<int>(r) << ';' << static_cast<int>(g) << ';'
           << static_cast<int>(b) << 'm';
    }

    inline void color(std::ostream& os, const core::ColorRGB8& color) {
        rgb(os, color.r, color.g, color.b);
    }

    inline auto scoped_rgb(uint8_t r, uint8_t g, uint8_t b) {
        return [=](std::ostream& os) { rgb(os, r, g, b); };
    }

    inline auto scoped_color(const core::ColorRGB8& c) {
        return [=](std::ostream& os) { color(os, c); };
    }
} // namespace fg

namespace bg {
    inline void rgb(std::ostream& os, uint8_t r, uint8_t g, uint8_t b) {
        os << "\x1b[48;2;" << static_cast<int>(r) << ';' << static_cast<int>(g) << ';'
           << static_cast<int>(b) << 'm';
    }

    inline void color(std::ostream& os, const core::ColorRGB8& color) {
        rgb(os, color.r, color.g, color.b);
    }

    inline auto scoped_rgb(uint8_t r, uint8_t g, uint8_t b) {
        return [=](std::ostream& os) { rgb(os, r, g, b); };
    }

    inline auto scoped_color(const core::ColorRGB8& c) {
        return [=](std::ostream& os) { color(os, c); };
    }
} // namespace bg

namespace cursor {
    inline void to(std::ostream& os, int row, int col) {
        os << "\x1b[" << row << ';' << col << 'H';
    }

    inline void up(std::ostream& os, int n) {
        os << "\x1b[" << n << 'A';
    }

    inline void down(std::ostream& os, int n) {
        os << "\x1b[" << n << 'B';
    }

    inline void forward(std::ostream& os, int n) {
        os << "\x1b[" << n << 'C';
    }

    inline void backward(std::ostream& os, int n) {
        os << "\x1b[" << n << 'D';
    }

    inline void save_position(std::ostream& os) {
        os << "\x1b[s";
    }

    inline void restore_position(std::ostream& os) {
        os << "\x1b[u";
    }

    inline void clear_screen(std::ostream& os) {
        os << "\x1b[2J";
    }

    inline void hide(std::ostream& os) {
        os << "\x1b[?25l";
    }

    inline void show(std::ostream& os) {
        os << "\x1b[?25h";
    }

} // namespace cursor

inline void enter_alternate_screen(std::ostream& os) {
    os << "\x1b[?1049h";
}

inline void exit_alternate_screen(std::ostream& os) {
    os << "\x1b[?1049l";
}

template<typename Func, typename... Applicators>
void scoped(std::ostream& os, Func&& func, Applicators&&... applicators) {
    (applicators(os), ...);
    func(os);
    reset(os);
}

} // namespace ansi
