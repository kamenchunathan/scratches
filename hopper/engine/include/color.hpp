#pragma once

#include <algorithm>
#include <cstdint>

namespace core {

struct ColorRGB8 {
    std::uint8_t r, g, b;

    static const ColorRGB8 BLACK;
    static const ColorRGB8 WHITE;
    static const ColorRGB8 RED;
    static const ColorRGB8 GREEN;
    static const ColorRGB8 BLUE;
    static const ColorRGB8 YELLOW;
    static const ColorRGB8 MAGENTA;
    static const ColorRGB8 CYAN;

    static ColorRGB8
    rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b

    ) {
        return ColorRGB8 {r, g, b};
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
    static const ColorRGBA8 BLACK;
    static const ColorRGBA8 WHITE;
    static const ColorRGBA8 RED;
    static const ColorRGBA8 GREEN;
    static const ColorRGBA8 BLUE;
    static const ColorRGBA8 YELLOW;
    static const ColorRGBA8 MAGENTA;
    static const ColorRGBA8 CYAN;
    static const ColorRGBA8 TRANSPARENT;

    static ColorRGBA8 rgba(
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a

    ) {
        return ColorRGBA8 {r, g, b, a};
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

    static const ColorRGBA32F BLACK;
    static const ColorRGBA32F WHITE;
    static const ColorRGBA32F RED;
    static const ColorRGBA32F GREEN;
    static const ColorRGBA32F BLUE;
    static const ColorRGBA32F YELLOW;
    static const ColorRGBA32F MAGENTA;
    static const ColorRGBA32F CYAN;
    static const ColorRGBA32F TRANSPARENT;

    static ColorRGBA32F rgba(
        float r,
        float g,
        float b,
        float a

    ) {
        return ColorRGBA32F {r, g, b, a};
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

inline const ColorRGB8 ColorRGB8::BLACK = {0, 0, 0};
inline const ColorRGB8 ColorRGB8::WHITE = {255, 255, 255};
inline const ColorRGB8 ColorRGB8::RED = {255, 0, 0};
inline const ColorRGB8 ColorRGB8::GREEN = {0, 255, 0};
inline const ColorRGB8 ColorRGB8::BLUE = {0, 0, 255};
inline const ColorRGB8 ColorRGB8::YELLOW = {255, 255, 0};
inline const ColorRGB8 ColorRGB8::MAGENTA = {255, 0, 255};
inline const ColorRGB8 ColorRGB8::CYAN = {0, 255, 255};

inline const ColorRGBA8 ColorRGBA8::BLACK = {0, 0, 0, 255};
inline const ColorRGBA8 ColorRGBA8::WHITE = {255, 255, 255, 255};
inline const ColorRGBA8 ColorRGBA8::RED = {255, 0, 0, 255};
inline const ColorRGBA8 ColorRGBA8::GREEN = {0, 255, 0, 255};
inline const ColorRGBA8 ColorRGBA8::BLUE = {0, 0, 255, 255};
inline const ColorRGBA8 ColorRGBA8::YELLOW = {255, 255, 0, 255};
inline const ColorRGBA8 ColorRGBA8::MAGENTA = {255, 0, 255, 255};
inline const ColorRGBA8 ColorRGBA8::CYAN = {0, 255, 255, 255};
inline const ColorRGBA8 ColorRGBA8::TRANSPARENT = {0, 0, 0, 0};

inline const ColorRGBA32F ColorRGBA32F::BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::RED = {1.0f, 0.0f, 0.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::MAGENTA = {1.0f, 0.0f, 1.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::CYAN = {0.0f, 1.0f, 1.0f, 1.0f};
inline const ColorRGBA32F ColorRGBA32F::TRANSPARENT = {0.0f, 0.0f, 0.0f, 0.0f};

} // namespace core
