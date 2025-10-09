#include <cassert>
#include <cstdio>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <utility>

#include "renderer/present.hpp"

class TerminalPresenter: public renderer::Presenter {
public:
    TerminalPresenter(FILE* output, const winsize& ws);
    ~TerminalPresenter() = default;

    void present(
        const renderer::FrameBuffer<renderer::CharacterPixel>& front_buffer,
        const renderer::FrameBuffer<renderer::CharacterPixel>& back_buffer
    );

    void init();
    void deinit();

    std::optional<std::pair<std::uint32_t, std::uint32_t>> size();

private:
    FILE* output_;
    winsize term_dim_;
    std::ostringstream buf_;

    void flush();
};

/* This is a redisplay algorithm that reduces the number of characters sent to the terminal on each frame
 * so that we hopefully reduce the time that rendering a frame takes. Idk really I haven't tested if full
 * redraws are a bottleneck
 *
 * Performs a multilevel myer's diff, first diffing line by line then character by character and converting
 * the edit script into optimized ANSI commands for the terminal.
 */
class Redisplay {
public:
    // std::vector<EditOperation> trace_to_edit_operations();
};
