#pragma once

#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <png.h>

#include "color.hpp"
#include "renderer/sprite.hpp"

namespace renderer {

std::optional<Sprite> load_png(const std::string& path) {
    auto file_closer = [](FILE* f) {
        if (f)
            fclose(f);
    };
    std::unique_ptr<FILE, decltype(file_closer)> png_file(fopen(path.c_str(), "rb"), file_closer);

    if (!png_file) {
        return std::nullopt;
    }

    PngReader reader;
    if (!reader) {
        return std::nullopt;
    }

    // libpng uses setjmp/longjmp for error handling.
    if (setjmp(png_jmpbuf(reader.png_ptr))) {
        return std::nullopt;
    }

    png_init_io(reader.png_ptr, png_file.get());
    png_read_info(reader.png_ptr, reader.info_ptr);

    png_uint_32 width = png_get_image_width(reader.png_ptr, reader.info_ptr);
    png_uint_32 height = png_get_image_height(reader.png_ptr, reader.info_ptr);
    png_byte color_type = png_get_color_type(reader.png_ptr, reader.info_ptr);
    png_byte bit_depth = png_get_bit_depth(reader.png_ptr, reader.info_ptr);

    if (bit_depth == 16)
        png_set_strip_16(reader.png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(reader.png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(reader.png_ptr);
    if (png_get_valid(reader.png_ptr, reader.info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(reader.png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(reader.png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(reader.png_ptr, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(reader.png_ptr, reader.info_ptr);

    Sprite sprite;
    sprite.width = width;
    sprite.height = height;
    std::vector<std::uint8_t> raw_data(width * height * 4);
    sprite.data.resize(width * height);

    std::vector<png_bytep> row_pointers(height);
    for (std::size_t y = 0; y < height; ++y) {
        row_pointers[y] = raw_data.data() + (y * width * 4);
    }

    png_read_image(reader.png_ptr, row_pointers.data());

    for (std::size_t i = 0; i < width * height; ++i) {
        std::size_t raw_idx = i * 4;
        sprite.data[i] = core::ColorRGBA8::rgba(
            raw_data[raw_idx],
            raw_data[raw_idx + 1],
            raw_data[raw_idx + 2],
            raw_data[raw_idx + 3]
        );
    }

    return sprite;
}

} // namespace renderer
