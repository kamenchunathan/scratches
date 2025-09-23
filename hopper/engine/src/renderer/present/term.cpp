#include <cstdio>
#include <memory>
#include <ostream>
#include <print>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "renderer/present/term.hpp"

namespace renderer {

TerminalPresenter::TerminalPresenter(std::ostream& output): output_(output) {
    if (isatty(STDOUT_FILENO)) {
        // Switch to alternate screen buffer and hide cursor
        output_ << "\x1b[?1049h\x1b[?25l" << std::flush;
    }
}

TerminalPresenter::~TerminalPresenter() {
    if (isatty(STDOUT_FILENO)) {
        // Restore screen buffer and show cursor
        output_ << "\x1b[?1049l\x1b[?25h" << std::flush;
    }
}

std::optional<std::pair<std::uint32_t, std::uint32_t>> TerminalPresenter::size() {
    if (!isatty(STDOUT_FILENO)) {
        return std::nullopt;
    }
    winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return std::nullopt;
    }
    // We can fit two pixels in every character cell, so we have double the rows.
    return std::make_pair(ws.ws_col, ws.ws_row * 2);
}

void TerminalPresenter::present(const FrameBuffer& front_buffer, const FrameBuffer&) {
    // Reset cursor to top-left
    output_ << "\x1b[H";

    for (std::uint32_t j = 0; j < front_buffer.pixel_height() - 1; j += 2) {
        for (std::uint32_t i = 0; i < front_buffer.pixel_width(); ++i) {
            const auto& upper_pixel_color =
                front_buffer.pixel_buf[j * front_buffer.pixel_width() + i];
            const auto& lower_pixel_color =
                front_buffer.pixel_buf[(j + 1) * front_buffer.pixel_width() + i];

            std::print(
                output_,
                "\x1b[38;2;{};{};{}m\x1b[48;2;{};{};{}m▀",
                static_cast<int>(upper_pixel_color.r * 255.0f),
                static_cast<int>(upper_pixel_color.g * 255.0f),
                static_cast<int>(upper_pixel_color.b * 255.0f),
                static_cast<int>(lower_pixel_color.r * 255.0f),
                static_cast<int>(lower_pixel_color.g * 255.0f),
                static_cast<int>(lower_pixel_color.b * 255.0f)
            );
        }
        output_ << '\n';
    }
    output_ << "\x1b[0m" << std::flush;
};

} // namespace renderer
