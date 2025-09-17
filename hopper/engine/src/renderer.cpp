#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>

#include "color.hpp"
#include "renderer.hpp"

namespace renderer {

void OutputBuffers::clear(core::ColorRGBA32F clear_color) {
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

void Renderer::present() {
    for (int j = 0; j < front_buffer.pixel_height() - 1; j += 2) {
        for (int i = 0; i < front_buffer.pixel_width(); ++i) {
            const auto& upper_pixel_color =
                front_buffer.pixel_buf[j * front_buffer.pixel_width() + i];
            const auto& lower_pixel_color =
                front_buffer.pixel_buf[(j + 1) * front_buffer.pixel_width() + i];

            std::cout << "\x1b[38;2;" << static_cast<int>(upper_pixel_color.r * 255.0f) << ";"
                      << static_cast<int>(upper_pixel_color.g * 255.0f) << ";"
                      << static_cast<int>(upper_pixel_color.b * 255.0f) << "m"
                      << "\x1b[48;2;" << static_cast<int>(lower_pixel_color.r * 255.0f) << ";"
                      << static_cast<int>(lower_pixel_color.g * 255.0f) << ";"
                      << static_cast<int>(lower_pixel_color.b * 255.0f) << "m"
                      << "▀";
        }
        std::cout << std::endl;
    }
    std::cout << "\x1b[0m" << std::flush;
}

} // namespace renderer
