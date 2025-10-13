#pragma once

#include <format>

#include "color.hpp"
#include "util/text.hpp"

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

template<>
struct std::formatter<renderer::CharacterPixel> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const renderer::CharacterPixel& p, std::format_context& ctx) const {
        return std::format_to(
            ctx.out(),
            "{} {} {}",
            util::to_utf8(p.codepoint),
            p.fg_color,
            p.bg_color
        );
    }
};
