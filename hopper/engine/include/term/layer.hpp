#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>

#include "application.hpp"
#include "input.hpp"
#include "term/platform.hpp"
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
    term::NativeTerminalState orig_state_;
    std::vector<char> input_buf_;
};

class TerminalLayer {
public:
    std::uint32_t frame_rate = 30;
    std::unique_ptr<Terminal> terminal;

    TerminalLayer() {
        TerminalLayer(std::make_unique<Terminal>(), 30);
    }
    TerminalLayer(std::unique_ptr<Terminal> terminal, std::uint32_t frame_rate):
        frame_rate(frame_rate),
        terminal(std::move(terminal)) {}

    void build(std::shared_ptr<core::Application> app);
    void run(std::shared_ptr<core::Application> app);
};
