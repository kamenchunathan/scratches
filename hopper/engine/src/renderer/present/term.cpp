#include <cstdio>
#include <print>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <utility>

#include "renderer/present/term.hpp"
#include "util/text.hpp"

namespace renderer {

TerminalPresenter::TerminalPresenter() {
    if (isatty(STDOUT_FILENO)) {
        // TODO: don't do this on creation
        // define a separate initialize method for presenters

        // Switch to alternate screen buffer and hide cursor
        std::print("\x1b[?1049h\x1b[?25l");
    }
}

TerminalPresenter::~TerminalPresenter() {
    if (isatty(STDOUT_FILENO)) {
        // Restore screen buffer and show cursor
        std::print("\x1b[?1049l\x1b[?25h");
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

void TerminalPresenter::present(
    const FrameBuffer<CharacterPixel>& front_buffer,
    const FrameBuffer<CharacterPixel>& /*back_buffer*/
) {
    // Reset cursor to top-left
    std::print("\x1b[H");

    const auto& front_data = front_buffer.data();
    const auto width = front_buffer.width();
    const auto height = front_buffer.height();

    for (std::uint32_t j = 0; j < height; ++j) {
        for (std::uint32_t i = 0; i < width; ++i) {
            const auto& pixel = front_data[j * width + i];

            std::print("\x1b[38;2;{};{};{}m", pixel.fg_color.r, pixel.fg_color.g, pixel.fg_color.b);
            std::print("\x1b[48;2;{};{};{}m", pixel.bg_color.r, pixel.bg_color.g, pixel.bg_color.b);
            std::print("{}", util::to_utf8(pixel.codepoint));
        }
        std::println("");
    }

    // Reset attributes
    std::print("\x1b[0m");
    getchar(); // Wait for input
}

} // namespace renderer
