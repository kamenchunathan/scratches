#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disable_raw_mode() {
    // Disable mouse reporting
    std::print("[?1006l");
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
}

void sigint_handler(int /*sig*/) {
    exit(0);
}

int main() {
    if (!isatty(STDIN_FILENO)) {
        std::println(stderr, "This program must be run in a terminal.");
        return 1;
    }

    enable_raw_mode();
    signal(SIGINT, sigint_handler);

    // Enable SGR mouse reporting
    std::print("\x1b[?1003h\x1b[?1006h");
    fflush(stdout);

    std::print("Reading mouse input. Press 'q' to quit.\r\n");

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            continue;
        }

        if (c == 'q') {
            break;
        }

        if (c == '\x1b') {
            char seq[64] = {0};
            // Read the rest of the sequence
            if (read(STDIN_FILENO, &seq[0], 1) != 1)
                continue;
            if (seq[0] != '[')
                continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1)
                continue;
            if (seq[1] != '<')
                continue;

            int i = 2;
            while (i < (int)sizeof(seq) - 1) {
                if (read(STDIN_FILENO, &seq[i], 1) != 1)
                    break;
                if (seq[i] == 'M' || seq[i] == 'm') {
                    break;
                }
                i++;
            }

            int button, x, y;
            char action_char = seq[i];
            seq[i] = '\0';

            if (sscanf(&seq[2], "%d;%d;%d", &button, &x, &y) == 3) {
                const char* action_str;
                if (action_char == 'm') {
                    action_str = "Release";
                } else {
                    switch (button) {
                        case 35:
                            action_str = "Hover";
                            break;
                        case 32:
                        case 33:
                        case 34:
                            action_str = "Drag";
                            button -= 32;
                            break;
                        default:
                            action_str = "Press";
                            break;
                    }
                }
                std::print("Mouse {}: button={}, x={}, y={}\r\n", action_str, button, x, y);
            }
        }
    }

    return 0;
}
