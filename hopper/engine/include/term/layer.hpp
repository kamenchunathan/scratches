#include <cassert>
#include <cstdio>
#include <memory>
#include <sys/ioctl.h>
#include <termios.h>
#include <vector>

#include "application.hpp"
#include "input.hpp"
#include "term/presenter.hpp"

/* Responsible for managing terminal state, attributes etc, creating a presenter to output to the terminal
 * and polling input from the terminal
 */
class Terminal {
public:
    Terminal(): Terminal(stdin, stdout) {}
    explicit Terminal(FILE* input, FILE* output);
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    std::unique_ptr<TerminalPresenter> presenter();

    std::vector<core::input::Event> poll_input();

private:
    FILE *input_, *output_;
    termios orig_termios_;
    std::vector<char> input_buf_;
};

class TerminalLayer {
public:
    std::uint32_t frame_rate = 30; // Set to lower value to avoid flickering
    std::unique_ptr<Terminal> terminal;

    void build(core::Application& app);
    void run(core::Application& app);
};
