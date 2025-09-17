#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

#include "color.hpp"

namespace renderer {

struct CharacterPixel {
    char32_t codepoint = U' ';
    core::ColorRGB8 fg_color;
    core::ColorRGB8 bg_color;
};

class OutputBuffers {
private:
    std::uint32_t char_height_, char_width_;
    std::uint32_t pixel_height_, pixel_width_;

public:
    std::vector<bool> char_mask;
    std::vector<CharacterPixel> char_buf;
    std::vector<core::ColorRGBA32F> pixel_buf;

    OutputBuffers(std::uint32_t canvas_height, std::uint32_t canvas_width):
        char_height_(canvas_height),
        char_width_(canvas_width),
        pixel_height_(canvas_height / 2),
        pixel_width_(canvas_width / 2),
        char_mask(char_height_ * char_width_),
        char_buf(char_height_ * char_width_),
        pixel_buf(pixel_height_ * pixel_width_) {}

    void clear(core::ColorRGBA32F clear_color);

    std::uint32_t char_height() const {
        return char_height_;
    }

    std::uint32_t char_width() const {
        return char_width_;
    }

    std::uint32_t pixel_height() const {
        return pixel_height_;
    }

    std::uint32_t pixel_width() const {
        return pixel_width_;
    }
};

class Renderer {
public:
    OutputBuffers front_buffer, back_buffer;

    Renderer(std::uint32_t canvas_width, std::uint32_t canvas_height):
        front_buffer(canvas_height, canvas_width),
        back_buffer(canvas_height, canvas_width) {
        assert(canvas_height % 2 == 0 && "The canvas height must be a multiple of 2");
    };

    void present();
};

} // namespace renderer
