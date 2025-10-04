#pragma once

#include "include/color.hpp"

#include <cstdint>
#include <iostream>

namespace ansi {

inline void reset(std::ostream& os) {
    os << "\033[0m";
}

namespace fg {
    inline void rgb(std::ostream& os, uint8_t r, uint8_t g, uint8_t b) {
        os << "\033[38;2;" << static_cast<int>(r) << ';' << static_cast<int>(g) << ';'
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
        os << "\033[48;2;" << r << ';' << g << ';' << b << 'm';
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
        os << "\033[" << row << ';' << col << 'H';
    }

    inline void up(std::ostream& os, int n) {
        os << "\033[" << n << 'A';
    }

    inline void down(std::ostream& os, int n) {
        os << "\033[" << n << 'B';
    }

    inline void forward(std::ostream& os, int n) {
        os << "\033[" << n << 'C';
    }

    inline void backward(std::ostream& os, int n) {
        os << "\033[" << n << 'D';
    }

    inline void save_position(std::ostream& os) {
        os << "\033[s";
    }

    inline void restore_position(std::ostream& os) {
        os << "\033[u";
    }

    inline void clear_screen(std::ostream& os) {
        os << "\033[2J";
    }

    inline void hide(std::ostream& os) {
        os << "\033[?25l";
    }

    inline void show(std::ostream& os) {
        os << "\033[?25h";
    }

} // namespace cursor

inline void enter_alternate_screen(std::ostream& os) {
    os << "\033[?1049h";
}

inline void exit_alternate_screen(std::ostream& os) {
    os << "\033[?1049l";
}

template<typename Func, typename... Applicators>
void scoped(std::ostream& os, Func&& func, Applicators&&... applicators) {
    (applicators(os), ...);
    func(os);
    reset(os);
}
} // namespace ansi
