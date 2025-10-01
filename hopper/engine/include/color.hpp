#pragma once

#include <cmath>
#include <cstdint>

namespace core {

struct ColorRGB8 {
    std::uint8_t r, g, b;

    // Editor highlighting
    static ColorRGB8
    rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b

    ) {
        return ColorRGB8 { .r = r, .g = g, .b = b };
    }
};

struct ColorRGBA8 {
    std::uint8_t r, g, b, a;

    static ColorRGBA8 rgba(
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a

    ) {
        return ColorRGBA8 { .r = r, .g = g, .b = b, .a = a };
    }
};

struct ColorRGBA32F {
    std::float_t r, g, b, a;

    static ColorRGBA32F rgba(
        std::float_t r,
        std::float_t g,
        std::float_t b,
        std::float_t a

    ) {
        return ColorRGBA32F { .r = r, .g = g, .b = b, .a = a };
    }
};

} // namespace core
