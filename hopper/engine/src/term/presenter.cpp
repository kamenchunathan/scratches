#include <cassert>
#include <cstddef>
#include <cstdio>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "term/ansi.hpp"
#include "term/presenter.hpp"
#include "util/text.hpp"

TerminalPresenter::TerminalPresenter(FILE* output, const winsize& ws):
    output_(output),
    term_dim_(ws) {}

std::optional<std::pair<std::uint32_t, std::uint32_t>> TerminalPresenter::size() {
    // TODO: Update this on change
    return std::make_pair(term_dim_.ws_col, term_dim_.ws_row);
}

void TerminalPresenter::init() {
    // TODO: Check bytes written and retry
    std::ostringstream buf;
    ansi::enter_alternate_screen(buf);
    ansi::cursor::hide(buf);
    write(fileno(output_), buf.str().c_str(), buf.str().size());
}

void TerminalPresenter::deinit() {
    // TODO: Check bytes written and retry
    std::ostringstream buf;
    ansi::cursor::show(buf);
    ansi::exit_alternate_screen(buf);
    write(fileno(output_), buf.str().c_str(), buf.str().size());
}

void TerminalPresenter::flush() {
    const std::string content = buf_.str();
    if (content.empty()) {
        return;
    }

    std::size_t total_written = 0;
    while (total_written < content.size()) {
        ssize_t bytes_written =
            write(fileno(output_), content.c_str() + total_written, content.size() - total_written);

        if (bytes_written >= 0) {
            total_written += bytes_written;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // We're just going to go into a tight loop here
                // TODO: Figure out alternative solutions for this
                continue;
            } else {
                // Some other error.
                ansi::scoped(
                    std::cerr,
                    [](std::ostream& s) { std::println(s, "Error writing to output"); },
                    ansi::fg::scoped_color(core::ColorRGB8::rgb(100, 10, 80))
                );
            }
            break;
        }
    }

    if (total_written >= content.size()) {
        // If everything was written, clear the stream for the next frame
        buf_.str("");
        buf_.clear();
    } else if (total_written > 0) {
        // If only part was written, store the remainder back in the stream
        std::string remaining_content = content.substr(total_written);
        buf_.str("");
        buf_.clear();
        buf_ << remaining_content;
    }
}

void TerminalPresenter::present(
    const renderer::FrameBuffer<renderer::CharacterPixel>& front_buffer,
    const renderer::FrameBuffer<renderer::CharacterPixel>& /*back_buffer*/
) {
    auto term_size_opt = size();

    const auto term_width = term_size_opt->first;
    const auto term_height = term_size_opt->second;
    const auto buffer_width = front_buffer.width();
    const auto buffer_height = front_buffer.height();

    std::uint32_t start_col = 1;
    std::uint32_t start_row = 1;

    if (term_width > buffer_width) {
        start_col = (term_width - buffer_width) / 2 + 1;
    }
    if (term_height > buffer_height) {
        start_row = (term_height - buffer_height) / 2 + 1;
    }

    ansi::cursor::to(buf_, start_row, start_col);

    const auto& front_data = front_buffer.data();
    const auto width = front_buffer.width();
    const auto height = front_buffer.height();

    for (std::uint32_t j = 0; j < height; ++j) {
        for (std::uint32_t i = 0; i < width; ++i) {
            const auto& pixel = front_data[j * width + i];

            ansi::scoped(
                buf_,
                [pixel](std::ostream& os) { os << util::to_utf8(pixel.codepoint); },
                ansi::fg::scoped_color(pixel.fg_color),
                ansi::bg::scoped_color(pixel.bg_color)
            );
        }
        ansi::cursor::to(buf_, start_row + j + 1, start_col);
    }

    flush();
}
