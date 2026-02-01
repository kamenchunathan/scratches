#pragma once

#include <optional>

#include "color.hpp"
#include "renderer/texture.hpp"

#ifndef __EMSCRIPTEN__

    #include "png.h"

namespace assets {

class PngReader {
public:
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    PngReader() {
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (png_ptr) {
            info_ptr = png_create_info_struct(png_ptr);
        }
    }

    ~PngReader() {
        if (png_ptr) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        }
    }

    PngReader(const PngReader&) = delete;
    PngReader& operator=(const PngReader&) = delete;
    PngReader(PngReader&&) = delete;
    PngReader& operator=(PngReader&&) = delete;

    operator bool() const {
        return png_ptr && info_ptr;
    }
};

std::optional<renderer::Texture2D<core::ColorRGBA8>> load_png(const std::string& path) {
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

    std::vector<core::ColorRGBA8> texture_data(width * height);
    std::vector<png_bytep> row_pointers(height);
    std::vector<uint8_t> raw_data(width * height * 4);

    for (std::size_t y = 0; y < height; ++y) {
        row_pointers[y] = raw_data.data() + (y * width * 4);
    }

    png_read_image(reader.png_ptr, row_pointers.data());

    for (std::size_t i = 0; i < width * height; ++i) {
        texture_data[i].r = raw_data[i * 4 + 0];
        texture_data[i].g = raw_data[i * 4 + 1];
        texture_data[i].b = raw_data[i * 4 + 2];
        texture_data[i].a = raw_data[i * 4 + 3];
    }

    return renderer::Texture2D<core::ColorRGBA8>(width, height, std::move(texture_data));
}

#endif // !__EMSCRIPTEN__

} // namespace asset
