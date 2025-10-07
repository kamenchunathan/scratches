#include <cerrno>
#include <csignal>
#include <cstddef>
#include <fcntl.h>
#include <print>
#include <string_view>
#include <termios.h>
#include <thread>
#include <variant>

#include "application.hpp"
#include "term/ansi.hpp"
#include "term/input_manager.hpp"

template<class... Ts>
struct overload: Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overload(Ts...) -> overload<Ts...>;

struct termios orig_termios;
int orig_flags;

void disable_raw_mode() {
    // Disable mouse reporting
    ansi::mouse_input_tracking(
        std::cout,
        false,
        ansi::MouseInputMode::AnyEvent,
        ansi::MouseEncodingMode::SGR_Extended
    );
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    raw.c_oflag |= OPOST; // Enable output processing for \n -> \r\n

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void sigint_handler(int /*sig*/) {
    exit(0);
}

class TerminalInputLayer {
public:
    void build(core::Application& app) {
        if (!isatty(STDIN_FILENO)) {
            std::println(stderr, "This program must be run in a terminal.");
            return;
        }

        enable_raw_mode();
        signal(SIGINT, sigint_handler);

        ansi::mouse_input_tracking(
            std::cout,
            true,
            ansi::MouseInputMode::AnyEvent,
            ansi::MouseEncodingMode::SGR_Extended
        );
        fflush(stdout);

        std::print("Reading mouse input. Press 'q' to quit.\r\n");

        app.set_runner(run);
    }

    static void run(core::Application&) {
        char buf[1024];

        while (true) {
            std::ptrdiff_t bytes_read = read(STDIN_FILENO, &buf, 1024);
            if (bytes_read > 0) {
                InputParser input_parser(std::string_view(buf, bytes_read));
                auto input_events = input_parser.parse();
                for (auto ev: input_events) {
                    std::visit(
                        overload {
                            [](core::input::KeyEvent kev) {
                                if (kev.code == core::input::KeyCode::Q) {
                                    exit(0);
                                }
                            },
                            [](core::input::MouseEvent) {},
                            [](core::input::ResizeEvent) {}
                        },
                        ev
                    );

                    std::println("{}", ev);
                }
            } else {
                if (errno != EAGAIN) {
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
        }
    }
};

int main() {
    core::Application app;
    app.add_layer(TerminalInputLayer {});
    app.run();
}
