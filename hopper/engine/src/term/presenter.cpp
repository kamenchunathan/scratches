#include <cassert>
#include <poll.h>
#include <span>
#include <unistd.h>

#include "log.hpp"
#include "profile.hpp"

#include "term/emitter.hpp"
#include "term/presenter.hpp"
#include "util/text.hpp"

TerminalPresenter::TerminalPresenter(FILE* output, const winsize& ws):
    output_(output),
    term_dim_(ws) {
    storage_.reserve(512 * 1024);
}

std::optional<std::pair<std::uint32_t, std::uint32_t>> TerminalPresenter::size() {
    return std::make_pair(term_dim_.ws_col, term_dim_.ws_row);
}

void TerminalPresenter::init() {
    term::TerminalEmitter emitter(storage_);
    emitter.append("\x1b[?1049h"); // enter_alternate_screen
    emitter.append("\x1b[?25l"); // cursor::hide
    flush();
}

void TerminalPresenter::deinit() {
    term::TerminalEmitter emitter(storage_);
    emitter.append("\x1b[?25h"); // cursor::show
    emitter.append("\x1b[?1049l"); // exit_alternate_screen
    flush();
}

void TerminalPresenter::flush() {
    HOPPER_ZONE_NAMED("TerminalPresenter::flush");
    HOPPER_PLOT("Terminal Buffer Size", storage_.size());
    if (storage_.empty()) {
        return;
    }
    HOPPER_DEBUG("terminal", "Storage size: {}", storage_.size());

    std::size_t total_written = 0;
    while (total_written < storage_.size()) {
        ssize_t bytes_written = write(
            fileno(output_),
            storage_.data() + total_written,
            storage_.size() - total_written
        );

        if (bytes_written >= 0) {
            total_written += bytes_written;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd = {fileno(output_), POLLOUT, 0};
                poll(&pfd, 1, -1);
                continue;
            } else {
                // Error writing
                break;
            }
        }
    }

    storage_.clear();
}

void TerminalPresenter::render_full_frame(
    const std::span<const renderer::CharacterPixel> buffer,
    std::size_t buffer_width,
    std::size_t buffer_height
) {
    HOPPER_ZONE_NAMED("render_full_frame");
    term::TerminalEmitter emitter(storage_);

    auto term_size_opt     = size();
    const auto term_width  = term_size_opt->first;
    const auto term_height = term_size_opt->second;

    std::uint32_t start_col = 1;
    std::uint32_t start_row = 1;

    if (term_width > buffer_width) {
        start_col = (term_width - buffer_width) / 2 + 1;
    }
    if (term_height > buffer_height) {
        start_row = (term_height - buffer_height) / 2 + 1;
    }

    for (std::uint32_t j = 0; j < buffer_height; ++j) {
        emitter.move_cursor(start_row + j, start_col);

        for (std::uint32_t i = 0; i < buffer_width;) {
            const auto& pixel = buffer[j * buffer_width + i];

            // Character RLE
            std::uint32_t count = 1;
            while (i + count < buffer_width && buffer[j * buffer_width + i + count] == pixel) {
                count++;
            }

            emitter.set_fg(pixel.fg_color);
            emitter.set_bg(pixel.bg_color);

            emitter.append_utf8(pixel.codepoint);
            if (count > 1) {
                emitter.repeat(count);
            }

            i += count;
        }
    }
    emitter.reset();
}

// render_diff is currently unused and would need a full rewrite to use TerminalEmitter efficiently.
// Keeping a skeleton for now or it can be removed.
void TerminalPresenter::render_diff(
    const std::vector<renderer::CharacterPixel>&,
    const std::vector<renderer::CharacterPixel>&,
    std::uint32_t,
    std::uint32_t
) {
    // TODO: Implement optimized diff rendering
}

void TerminalPresenter::present(
    const std::span<const renderer::CharacterPixel> front_buffer,
    const std::span<const renderer::CharacterPixel> back_buffer,
    std::size_t width,
    std::size_t height

) {
    HOPPER_ZONE_NAMED("TerminalPresenter::present");
    assert(front_buffer.size() == width * height && front_buffer.size() == back_buffer.size());
    render_full_frame(front_buffer, width, height);
    flush();
}
