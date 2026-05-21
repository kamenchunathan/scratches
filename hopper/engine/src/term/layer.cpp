#include <chrono>
#include <csignal>
#include <cstdio>
#include <thread>

#include "log.hpp"
#include "profile.hpp"
#include "term/ansi.hpp"
#include "term/input_parser.hpp"
#include "term/layer.hpp"
#include "term/platform.hpp"

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>

volatile std::sig_atomic_t term_size_changed;

void handle_sigwinch(int) {
    term_size_changed = 1;
}

#endif

Terminal::Terminal(FILE* input, FILE* output): input_(input), output_(output) {
    // NOTE: Important that the input buffer has capacity for reading into it
    // with the read call which requires a pointer
    input_buf_.resize(256);

#if defined(PLATFORM_LINUX)
    auto input_fd = fileno(input_);
    if (!isatty(input_fd)) {
        HOPPER_ERROR("terminal", "file provided is not a terminal");
        return;
    }

    struct termios original_termios;
    auto res = tcgetattr(input_fd, &original_termios);

    if (res == -1) {
        HOPPER_ERROR("terminal", "could not get original termios");
        return;
    }
    orig_state_ = original_termios;

    // Enable raw mode, immediate input processing and not echoing characters
    struct termios current_termios;
    cfmakeraw(&current_termios);
    tcsetattr(input_fd, TCSANOW, &current_termios);

    // Set non-blocking
    int flags = fcntl(input_fd, F_GETFL);
    if (fcntl(input_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        HOPPER_ERROR("terminal", "Error setting terminal flags");
    }

    signal(SIGWINCH, handle_sigwinch);

#elif defined(PLATFORM_WINDOWS)
    HANDLE hIn  = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(input_)));
    HANDLE hOut = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(output_)));

    if (hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE) {
        HOPPER_ERROR("terminal", "input or output handle for terminal provided is invalid");
        return;
    }

    GetConsoleMode(hIn, &orig_state_.dwInputMode);
    GetConsoleMode(hOut, &orig_state_.dwOutputMode);
    orig_state_.cpInput  = GetConsoleCP();
    orig_state_.cpOutput = GetConsoleOutputCP();

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    DWORD dwInputMode = orig_state_.dwInputMode;
    dwInputMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    dwInputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hIn, dwInputMode);

    DWORD dwOutputMode = orig_state_.dwOutputMode;
    dwOutputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(hOut, dwOutputMode);
#endif

    // TODO: Rework ansi code
    ansi::mouse_input_tracking(
        std::cout,
        true,
        ansi::MouseInputMode::AnyEvent,
        ansi::MouseEncodingMode::SGR_Extended
    );
    fflush(output_);
}

Terminal::~Terminal() {
    // TODO: Rework ansi code
    ansi::mouse_input_tracking(
        std::cout,
        false,
        ansi::MouseInputMode::AnyEvent,
        ansi::MouseEncodingMode::SGR_Extended
    );
    fflush(output_);

#if defined(PLATFORM_LINUX)
    tcsetattr(fileno(input_), TCSANOW, &orig_state_);

#elif defined(PLATFORM_WINDOWS)
    HANDLE hIn  = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(input_)));
    HANDLE hOut = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(output_)));

    SetConsoleMode(hIn, orig_state_.dwInputMode);
    SetConsoleMode(hOut, orig_state_.dwOutputMode);
    SetConsoleCP(orig_state_.cpInput);
    SetConsoleOutputCP(orig_state_.cpOutput);
#endif
}

std::unique_ptr<TerminalPresenter> Terminal::presenter() {
    term::TerminalSize ts;

#if defined(PLATFORM_LINUX)
    winsize ws;
    ioctl(fileno(output_), TIOCGWINSZ, &ws);
    ts.width  = ws.ws_col;
    ts.height = ws.ws_row;

#elif defined(PLATFORM_WINDOWS)
    HANDLE hOut = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(output_)));
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    ts.width  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    ts.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

#endif

    return std::make_unique<TerminalPresenter>(output_, ts);
}

std::vector<core::input::Event> Terminal::poll_input() {
#if defined(PLATFORM_LINUX)
    ssize_t bytes_read = read(fileno(input_), input_buf_.data(), input_buf_.capacity());

    if (bytes_read > 0) {
        InputParser parser(
            std::string_view {input_buf_.data(), static_cast<std::size_t>(bytes_read)}
        );
        return parser.parse();
    }
#elif defined(PLATFORM_WINDOWS)
    HANDLE hIn = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(input_)));
    DWORD dwPending;
    if (GetNumberOfConsoleInputEvents(hIn, &dwPending) && dwPending > 0) {
        // This is a bit complex as we might get ANSI sequences mixed with Windows events
        // For now, let's try reading as a byte stream if possible, or use ReadConsoleInput
        // and convert. However, the existing InputParser expects ANSI sequences.
        // Modern Windows Terminal sends ANSI sequences.

        // We use ReadFile on the console handle to get the raw byte stream (ANSI)
        DWORD bytes_read;
        if (ReadFile(
                hIn,
                input_buf_.data(),
                static_cast<DWORD>(input_buf_.capacity()),
                &bytes_read,
                nullptr
            )
            && bytes_read > 0)
        {
            InputParser parser(
                std::string_view {input_buf_.data(), static_cast<std::size_t>(bytes_read)}
            );
            return parser.parse();
        }
    }
#endif

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

        HOPPER_FRAME_MARK();
        auto work_duration = std::chrono::high_resolution_clock::now() - frame_start_time;
        if (work_duration < target_frame_duration) {
            std::this_thread::sleep_for(target_frame_duration - work_duration);
        }
    }
}
