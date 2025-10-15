#pragma once

#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <format>
#include <vector>

#include <png.h>

#include "color.hpp"

namespace renderer {

struct Sprite {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<core::ColorRGBA8> data;
};

std::optional<Sprite> load_png(const std::string& path);

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

} // namespace renderer

template<>
struct std::formatter<renderer::Sprite> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const renderer::Sprite& sprite, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "Sprite({}x{})", sprite.width, sprite.height);
    }
};
