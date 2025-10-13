#pragma once

#include <cassert>
#include <cstdio>
#include <optional>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <utility>
#include <vector>

#include "renderer/present.hpp"
#include "term/diff.hpp"

/* This is a redisplay algorithm that reduces the number of characters sent to the terminal on each frame
 * so that we hopefully reduce the time that rendering a frame takes. Idk really I haven't tested if full
 * redraws are a bottleneck
 *
 * Performs a multilevel myer's diff, first diffing line by line then character by character and converting
 * the edit script into optimized ANSI commands for the terminal.
 */

class TerminalPresenter: public renderer::Presenter {
public:
    TerminalPresenter(FILE* output, const winsize& ws);
    ~TerminalPresenter() = default;

    void present(
        const renderer::FrameBuffer<renderer::CharacterPixel>& front_buffer,
        const renderer::FrameBuffer<renderer::CharacterPixel>& back_buffer
    ) override;

    void init() override;
    void deinit() override;

    std::optional<std::pair<std::uint32_t, std::uint32_t>> size();

private:
    FILE* output_;
    winsize term_dim_;
    std::ostringstream buf_;

    void flush();
    void render_full_frame(const renderer::FrameBuffer<renderer::CharacterPixel>& buffer);
    void render_diff(
        const std::vector<renderer::CharacterPixel>& old_data,
        const std::vector<renderer::CharacterPixel>& new_data,
        std::uint32_t width,
        std::uint32_t height
    );
};
