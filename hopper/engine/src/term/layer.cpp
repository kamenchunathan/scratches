#include <cerrno>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <print>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "term/ansi.hpp"
#include "term/layer.hpp"

volatile std::sig_atomic_t term_size_changed;

void handle_sigwinch(int) {
    term_size_changed = 1;
}

Terminal::Terminal(FILE* input, FILE* output): input_(input), output_(output) {
    // TODO: Enable mouse input tracking
    auto input_fd = fileno(input_);
    if (!isatty(input_fd)) {
        // TODO: Add engine wide logging framework
        ansi::scoped(
            std::cerr,
            [](std::ostream& s) { std::println(s, "file provided is not a terminal"); },
            ansi::fg::scoped_color(core::ColorRGB8::rgb(100, 0, 0))
        );
        return;
    }

    struct termios original_termios;
    auto res = tcgetattr(input_fd, &original_termios);
    if (res == -1) {
        ansi::scoped(
            std::cerr,
            [](std::ostream& s) { std::println(s, "could not get original termios"); },
            ansi::fg::scoped_color(core::ColorRGB8::rgb(100, 0, 0))
        );
        return;
    }
    orig_termios_ = original_termios;

    // Enable raw mode, immediate input processing and not echoing characters
    struct termios current_termios;
    cfmakeraw(&current_termios);
    tcsetattr(input_fd, TCSANOW, &current_termios);

    // Set non-blocking
    int flags = fcntl(input_fd, F_GETFL);
    if (fcntl(input_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        ansi::scoped(
            std::cerr,
            [](std::ostream& s) { std::println(s, "Error in setting flags"); },
            ansi::fg::scoped_color(core::ColorRGB8::rgb(100, 0, 0))
        );
    }

    signal(SIGWINCH, handle_sigwinch);
}

Terminal::~Terminal() {
    auto input_fd = fileno(input_);
    tcsetattr(input_fd, TCSANOW, &orig_termios_);
}

std::unique_ptr<TerminalPresenter> Terminal::presenter() {
    struct winsize ws;
    ioctl(fileno(output_), TIOCGWINSZ, &ws);
    return std::make_unique<TerminalPresenter>(output_, ws);
}
