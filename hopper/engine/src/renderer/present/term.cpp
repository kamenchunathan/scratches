#include <cstdio>
#include <memory>
#include <print>

#include "renderer/present/term.hpp"

namespace renderer {

TerminalPresenter::TerminalPresenter() {}

TerminalPresenter::~TerminalPresenter() {}

void TerminalPresenter::present(const OutputBuffers& front_buffer, const OutputBuffers&) {
    for (int j = 0; j < front_buffer.pixel_height() - 1; j += 2) {
        for (int i = 0; i < front_buffer.pixel_width(); ++i) {
            const auto& upper_pixel_color =
                front_buffer.pixel_buf[j * front_buffer.pixel_width() + i];
            const auto& lower_pixel_color =
                front_buffer.pixel_buf[(j + 1) * front_buffer.pixel_width() + i];

            std::print(
                "[38;2;{};{};{}m[48;2;{};{};{}m▀",
                static_cast<int>(upper_pixel_color.r * 255.0f),
                static_cast<int>(upper_pixel_color.g * 255.0f),
                static_cast<int>(upper_pixel_color.b * 255.0f),
                static_cast<int>(lower_pixel_color.r * 255.0f),
                static_cast<int>(lower_pixel_color.g * 255.0f),
                static_cast<int>(lower_pixel_color.b * 255.0f)
            );
        }
        std::println("");
    }
    std::print("[0m");
    std::fflush(stdout);
};

} // namespace renderer
