#pragma once

#include "color.hpp"

namespace renderer {

struct CharacterPixel {
    char32_t codepoint = U' ';
    core::ColorRGB8 fg_color;
    core::ColorRGB8 bg_color;

    bool operator==(const CharacterPixel& other) const {
        return codepoint == other.codepoint && fg_color == other.fg_color
            && bg_color == other.bg_color;
    }

    bool operator!=(const CharacterPixel& other) const {
        return codepoint != other.codepoint || fg_color != other.fg_color
            || bg_color != other.bg_color;
    }
};

} // namespace renderer
