#pragma once

#include "color.hpp"

namespace renderer {

struct CharacterPixel {
    char32_t codepoint = U' ';
    core::ColorRGB8 fg_color;
    core::ColorRGB8 bg_color;
};

} // namespace renderer
