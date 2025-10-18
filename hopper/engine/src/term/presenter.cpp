#include <print>
#include <unistd.h>

#include "term/ansi.hpp"
#include "term/presenter.hpp"
#include "util/text.hpp"

template<typename T>
using EditSequence = std::vector<Edit<T>>;

TerminalPresenter::TerminalPresenter(FILE* output, const winsize& ws):
    output_(output),
    term_dim_(ws) {}

std::optional<std::pair<std::uint32_t, std::uint32_t>> TerminalPresenter::size() {
    return std::make_pair(term_dim_.ws_col, term_dim_.ws_row);
}

void TerminalPresenter::init() {
    std::ostringstream buf;
    ansi::enter_alternate_screen(buf);
    ansi::cursor::hide(buf);
    write(fileno(output_), buf.str().c_str(), buf.str().size());
}

void TerminalPresenter::deinit() {
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
                continue;
            } else {
                // Error writing
                break;
            }
        }
    }

    if (total_written >= content.size()) {
        buf_.str("");
        buf_.clear();
    } else if (total_written > 0) {
        std::string remaining_content = content.substr(total_written);
        buf_.str("");
        buf_.clear();
        buf_ << remaining_content;
    }
}

void TerminalPresenter::render_full_frame(
    const renderer::FrameBufferPrev<renderer::CharacterPixel>& buffer
) {
    auto term_size_opt = size();
    const auto term_width = term_size_opt->first;
    const auto term_height = term_size_opt->second;
    const auto buffer_width = buffer.width();
    const auto buffer_height = buffer.height();

    std::uint32_t start_col = 1;
    std::uint32_t start_row = 1;

    if (term_width > buffer_width) {
        start_col = (term_width - buffer_width) / 2 + 1;
    }
    if (term_height > buffer_height) {
        start_row = (term_height - buffer_height) / 2 + 1;
    }

    ansi::cursor::to(buf_, start_row, start_col);

    const auto& data = buffer.data();
    const auto width = buffer.width();
    const auto height = buffer.height();

    for (std::uint32_t j = 0; j < height; ++j) {
        for (std::uint32_t i = 0; i < width; ++i) {
            const auto& pixel = data[j * width + i];

            ansi::scoped(
                buf_,
                [pixel](std::ostream& os) { os << util::to_utf8(pixel.codepoint); },
                ansi::fg::scoped_color(pixel.fg_color),
                ansi::bg::scoped_color(pixel.bg_color)
            );
        }
        if (j < height - 1) {
            ansi::cursor::to(buf_, start_row + j + 1, start_col);
        }
    }
}

void generate_ansi_for_line(
    std::ostream& os,
    const EditSequence<renderer::CharacterPixel>& edits,
    std::uint32_t start_row,
    std::uint32_t start_col
) {
    std::uint32_t pos = 0;
    for (const auto& edit: edits) {
        std::visit(
            [&](const auto& e) {
                using EditType = std::decay_t<decltype(e)>;

                if constexpr (std::is_same_v<EditType, Delete>) {
                    ansi::cursor::to(os, start_row, start_col + pos);
                    ansi::edit::delete_char(os, 1);

                } else if constexpr (std::is_same_v<EditType, Insert<renderer::CharacterPixel>>) {
                    // Insert blank space
                    ansi::cursor::to(os, start_row, start_col + pos);
                    ansi::edit::insert_char(os, 1);

                    // Render the inserted character with styling
                    ansi::cursor::to(os, start_row, start_col + pos);
                    ansi::scoped(
                        os,
                        [&e](std::ostream& stream) { stream << util::to_utf8(e.value.codepoint); },
                        ansi::fg::scoped_color(e.value.fg_color),
                        ansi::bg::scoped_color(e.value.bg_color)
                    );

                    // Move forward
                    pos++;

                } else {
                    // Match: move forward
                    pos++;
                }
            },
            edit
        );
    }
}

// Main render_diff implementation
void TerminalPresenter::render_diff(
    const std::vector<renderer::CharacterPixel>& new_data,
    const std::vector<renderer::CharacterPixel>& old_data,
    std::uint32_t width,
    std::uint32_t height
) {
    auto term_size_opt = size();
    const auto term_width = term_size_opt->first;
    const auto term_height = term_size_opt->second;

    std::uint32_t start_col = 1;
    std::uint32_t start_row = 1;

    if (term_width > width) {
        start_col = (term_width - width) / 2 + 1;
    }
    if (term_height > height) {
        start_row = (term_height - height) / 2 + 1;
    }

    // Find changed lines
    std::vector<std::uint32_t> changed_lines;
    for (std::uint32_t y = 0; y < height; ++y) {
        bool line_changed = false;
        for (std::uint32_t x = 0; x < width; ++x) {
            if (old_data[y * width + x] != new_data[y * width + x]) {
                line_changed = true;
                break;
            }
        }
        if (line_changed) {
            changed_lines.push_back(y);
        }
    }

    // For each changed line, use Myers diff to generate minimal edits
    for (std::uint32_t y: changed_lines) {
        std::vector<renderer::CharacterPixel> old_line(
            old_data.begin() + y * width,
            old_data.begin() + (y + 1) * width
        );
        std::vector<renderer::CharacterPixel> new_line(
            new_data.begin() + y * width,
            new_data.begin() + (y + 1) * width
        );

        MyersDiff<renderer::CharacterPixel> differ(old_line, new_line);
        std::vector<Snake> snakes;
        Box box(0, 0, old_line.size(), new_line.size());
        differ.build_trace(box, snakes);

        EditSequence<renderer::CharacterPixel> script =
            build_edit_sequence(old_line, new_line, snakes);

        const std::uint32_t line_row = start_row + y;
        generate_ansi_for_line(buf_, script, line_row, start_col);
    }

    ansi::reset(buf_);
}

void TerminalPresenter::present(
    const renderer::FrameBufferPrev<renderer::CharacterPixel>& front_buffer,
    const renderer::FrameBufferPrev<renderer::CharacterPixel>& back_buffer
) {
    // const auto& front_buffer_data = front_buffer.data();
    // const auto& back_buffer_data = back_buffer.data();
    // const auto width = front_buffer.width();
    // const auto height = front_buffer.height();

    // The diffed rendering is artefacted and my be slower than rendering full frames
    // so it's not turned on for now
    // render_diff(front_buffer_data, back_buffer_data, width, height);
    // TODO: Set the render backend used as an option definable by arguments
    render_full_frame(front_buffer);
    flush();
}
