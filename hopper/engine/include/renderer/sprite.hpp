#pragma once

#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <format>

#include <png.h>

namespace renderer {

struct Sprite {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
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
