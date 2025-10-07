#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <sys/ioctl.h>
#include <termios.h>
#include <vector>

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

private:
    FILE *input_, *output_;
    termios orig_termios_;
    std::vector<std::byte> input_buf_;
};
