#include "renderer/types.hpp"

#include <algorithm>

#include "color.hpp"

namespace renderer {

void FrameBuffer::clear(core::ColorRGBA32F clear_color) {
    std::fill(pixel_buf.begin(), pixel_buf.end(), clear_color);

    const core::ColorRGB8 char_clear_color {
        .r = static_cast<std::uint8_t>(clear_color.r * 255.0f),
        .g = static_cast<std::uint8_t>(clear_color.g * 255.0f),
        .b = static_cast<std::uint8_t>(clear_color.b * 255.0f),
    };

    // Should not matter because the mask is set, Consider using a debug color to check for errors
    std::fill(
        char_buf.begin(),
        char_buf.end(),
        CharacterPixel {
            .codepoint = U' ',
            .fg_color = char_clear_color,
            .bg_color = char_clear_color,
        }
    );
    std::fill(char_mask.begin(), char_mask.end(), false);
}

} // namespace renderer
