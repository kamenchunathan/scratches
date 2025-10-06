#pragma once

#include <algorithm>
#include <cstdint>

namespace core {

struct ColorRGB8 {
    std::uint8_t r, g, b;

    static ColorRGB8
    rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b

    ) {
        return ColorRGB8 { .r = r, .g = g, .b = b };
    }

    ColorRGB8 operator*(float t) const {
        return {
            static_cast<std::uint8_t>(r * t),
            static_cast<std::uint8_t>(g * t),
            static_cast<std::uint8_t>(b * t),
        };
    }

    ColorRGB8 operator+(const ColorRGB8& other) const {
        return {
            static_cast<std::uint8_t>(std::min(255, r + other.r)),
            static_cast<std::uint8_t>(std::min(255, g + other.g)),
            static_cast<std::uint8_t>(std::min(255, b + other.b)),
        };
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

    ColorRGBA8 operator*(float t) const {
        return {
            .r = static_cast<std::uint8_t>(r * t),
            .g = static_cast<std::uint8_t>(g * t),
            .b = static_cast<std::uint8_t>(b * t),
            .a = static_cast<std::uint8_t>(a * t),
        };
    }

    ColorRGBA8 operator+(const ColorRGBA8& other) const {
        return {
            static_cast<std::uint8_t>(std::min(255, r + other.r)),
            static_cast<std::uint8_t>(std::min(255, g + other.g)),
            static_cast<std::uint8_t>(std::min(255, b + other.b)),
            static_cast<std::uint8_t>(std::min(255, a + other.a)),
        };
    }
};

struct ColorRGBA32F {
    float r, g, b, a;

    static ColorRGBA32F rgba(
        float r,
        float g,
        float b,
        float a

    ) {
        return ColorRGBA32F { .r = r, .g = g, .b = b, .a = a };
    }

    ColorRGBA32F operator*(float t) const {
        return {
            r * t,
            g * t,
            b * t,
            a * t,
        };
    }

    ColorRGBA32F operator+(const ColorRGBA32F& other) const {
        return {
            r + other.r,
            g + other.g,
            b + other.b,
            a + other.a,
        };
    }
};

} // namespace core
