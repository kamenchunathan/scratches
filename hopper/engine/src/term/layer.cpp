#include <chrono>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <print>
#include <sys/ioctl.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

#include "input.hpp"
#include "term/ansi.hpp"
#include "term/input_parser.hpp"
#include "term/layer.hpp"

volatile std::sig_atomic_t term_size_changed;

void handle_sigwinch(int) {
    term_size_changed = 1;
}

Terminal::Terminal(FILE* input, FILE* output): input_(input), output_(output) {
    input_buf_.resize(1024);

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

    ansi::mouse_input_tracking(
        std::cout,
        true,
        ansi::MouseInputMode::AnyEvent,
        ansi::MouseEncodingMode::SGR_Extended
    );
    fflush(output_);
}

Terminal::~Terminal() {
    ansi::mouse_input_tracking(
        std::cout,
        false,
        ansi::MouseInputMode::AnyEvent,
        ansi::MouseEncodingMode::SGR_Extended
    );
    fflush(output_);

    auto input_fd = fileno(input_);
    tcsetattr(input_fd, TCSANOW, &orig_termios_);
}

std::unique_ptr<TerminalPresenter> Terminal::presenter() {
    struct winsize ws;
    ioctl(fileno(output_), TIOCGWINSZ, &ws);
    return std::make_unique<TerminalPresenter>(output_, ws);
}

std::vector<core::input::Event> Terminal::poll_input() {
    ssize_t bytes_read = read(fileno(input_), input_buf_.data(), input_buf_.capacity());

    if (bytes_read > 0) {
        InputParser parser(
            std::string_view {input_buf_.data(), static_cast<std::size_t>(bytes_read)}
        );
        return parser.parse();
    }

    return {};
}

void TerminalLayer::build(std::shared_ptr<core::Application> app) {
    app->world.insert_resource(terminal->presenter());
    app->set_runner([this](std::shared_ptr<core::Application> app_ref) { run(app_ref); });
}

void TerminalLayer::run(std::shared_ptr<core::Application> app) {
    auto last_time                   = std::chrono::high_resolution_clock::now();
    const auto target_frame_duration = std::chrono::milliseconds(1000 / frame_rate);

    while (!app->should_exit()) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        auto delta_time       = frame_start_time - last_time;
        last_time             = frame_start_time;

        // Update the InputState resource
        auto events = terminal->poll_input();
        if (auto opt_input_state = app->world.get_resource<core::input::InputState>()) {
            opt_input_state->get().process_events(events);
        }

        app->tick(std::chrono::duration<double>(delta_time).count());

        auto work_duration = std::chrono::high_resolution_clock::now() - frame_start_time;
        if (work_duration < target_frame_duration) {
            std::this_thread::sleep_for(target_frame_duration - work_duration);
        }
    }
}
