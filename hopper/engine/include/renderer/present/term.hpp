#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <utility>
#include <vector>

#include "renderer/present.hpp"

namespace renderer {

class TerminalPresenter;

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

class TerminalPresenter: public Presenter {
public:
    TerminalPresenter(FILE* output, const winsize& ws);
    ~TerminalPresenter() = default;

    void present(
        const FrameBuffer<CharacterPixel>& front_buffer,
        const FrameBuffer<CharacterPixel>& back_buffer
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

/* Structures for the Myer's linear space diff implementation
 */
struct Box {
    std::int32_t x1, y1; // Top left corner
    std::int32_t x2, y2; // Bottom right corner

    Box(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2):
        x1(x1),
        y1(y1),
        x2(x2),
        y2(y2) {
        assert(x1 >= 0 && y1 >= 0 && x2 >= x1 && y2 >= y1);
    }

    [[nodiscard]] std::int32_t width() const {
        return x2 - x1;
    }

    [[nodiscard]] std::int32_t height() const {
        return y2 - y1;
    }

    [[nodiscard]] std::int32_t size() const {
        return width() + height();
    }

    [[nodiscard]] bool empty() const {
        return width() == 0 && height() == 0;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Box& box) {
    return os << "Box(x1: " << box.x1 << ", y1: " << box.y1 << ", x2: " << box.x2
              << ", y2: " << box.y2 << ")";
}

struct Snake {
    std::int32_t x_start, y_start;
    std::int32_t x_end, y_end;

    bool operator==(const Snake& other) const {
        return x_start == other.x_start && y_start == other.y_start && x_end == other.x_end
            && y_end == other.y_end;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Snake& snake) {
    return os << "Snake(x_start: " << snake.x_start << ", y_start: " << snake.y_start
              << ", x_end: " << snake.x_end << ", y_end: " << snake.y_end << ")";
}

/* An implementation of linear space myer's search algorithm based on the original
 * [Myer's Diff Algorithm paper](http://www.xmailserver.org/diff2.pdf)
 * and a [Series of tutorials](https://blog.jcoglan.com/2017/02/12/the-myers-diff-algorithm-part-1/) on building
 * git by James Coglan
 */
template<typename T>
class MyersDiff {
public:
    /* IMPORTANT: The references to these vectors should be alive for the entire lifetime of this class
     */
    explicit MyersDiff(const std::vector<T>& a, const std::vector<T>& b);

    std::vector<std::pair<std::uint32_t, std::uint32_t>> diff();

    // Public only for testing
    Snake find_middle_snake(const Box&);

private:
    void diff_recursive(const Box&, std::vector<std::pair<std::uint32_t, std::uint32_t>>&);

    std::vector<std::int32_t> forward_v_;
    std::vector<std::int32_t> reverse_v_;

    const std::vector<T>& a_;
    const std::vector<T>& b_;
};

template<typename T>
MyersDiff<T>::MyersDiff(const std::vector<T>& a, const std::vector<T>& b): a_(a), b_(b) {
    // For linear space algorithm, we need space for k values ranging from -max_d to +max_d
    // where max_d is at most (N + M + 1) / 2 + 1
    const std::size_t max_d = (a.size() + b.size() + 1) / 2 + 1;
    const std::size_t v_size = 2 * max_d + 1;

    forward_v_.resize(v_size, 0);
    reverse_v_.resize(v_size, 0);
}

template<typename T>
std::vector<std::pair<std::uint32_t, std::uint32_t>> MyersDiff<T>::diff() {
    if (a_.empty() && b_.empty()) {
        return {};
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> trace;
    diff_recursive(Box(0, 0, a_.size(), b_.size()), trace);
    return trace;
}

template<typename T>
void MyersDiff<T>::diff_recursive(
    const Box& box,
    std::vector<std::pair<std::uint32_t, std::uint32_t>>& trace
) {
    // Base cases
    if (box.empty()) {
        return;
    }

    // Only insertions
    if (box.width() == 0) {
        return;
    }
    // Only deletions
    if (box.height() == 0) {
        return;
    }

    Snake snake = find_middle_snake(box);

    // Recursively handle the region before the snake
    if (snake.x_start > box.x1 || snake.y_start > box.y1) {
        diff_recursive(Box(box.x1, box.y1, snake.x_start, snake.y_start), trace);
    }

    // Add the snake (matching elements) to the trace
    const std::int32_t snake_length = snake.x_end - snake.x_start;
    for (std::int32_t i = 0; i < snake_length; ++i) {
        trace.emplace_back(snake.x_start + i, snake.y_start + i);
    }

    // Recursively handle the region after the snake
    if (snake.x_end < box.x2 || snake.y_end < box.y2) {
        diff_recursive(Box(snake.x_end, snake.y_end, box.x2, box.y2), trace);
    }
}

template<typename T>
Snake MyersDiff<T>::find_middle_snake(const Box& box) {
    const std::int32_t n = box.width();
    const std::int32_t m = box.height();
    const std::int32_t delta = n - m;
    const std::int32_t max_d = (n + m + 1) / 2 + 1;

    // Base cases
    if (n == 0 && m == 0) {
        return Snake {box.x1, box.y1, box.x1, box.y1};
    }
    if (n == 0) {
        return Snake {box.x1, box.y1, box.x1, box.y1};
    }
    if (m == 0) {
        return Snake {box.x1, box.y1, box.x1, box.y1};
    }

    // V arrays are indexed by k, which can be negative. Use offset to map to positive indices.
    const std::int32_t offset = max_d;

    for (std::int32_t d = 0; d < max_d; ++d) {
        // Forward search
        for (std::int32_t k = -d; k <= d; k += 2) {
            std::int32_t x;

            // Choose direction: down (k == -d) or from k-1 diagonal, or right from k+1
            if (k == -d || (k != d && forward_v_[offset + k - 1] < forward_v_[offset + k + 1])) {
                x = forward_v_[offset + k + 1];
            } else {
                x = forward_v_[offset + k - 1] + 1;
            }
            std::int32_t y = x - k;

            // Record snake start
            const std::int32_t x_start = x;
            const std::int32_t y_start = y;

            // Extend diagonal as far as possible
            while (x < n && y < m && a_[box.x1 + x] == b_[box.y1 + y]) {
                x++;
                y++;
            }
            forward_v_[offset + k] = x;

            // Check for overlap with reverse search (odd delta case)
            if (delta % 2 != 0 && k >= delta - d + 1 && k <= delta + d - 1) {
                if (forward_v_[offset + k] + reverse_v_[offset + delta - k] >= n) {
                    return Snake {
                        .x_start = box.x1 + x_start,
                        .y_start = box.y1 + y_start,
                        .x_end = box.x1 + x,
                        .y_end = box.y1 + y,
                    };
                }
            }
        }

        // Reverse search
        for (std::int32_t k = -d; k <= d; k += 2) {
            std::int32_t x;

            if (k == -d || (k != d && reverse_v_[offset + k - 1] < reverse_v_[offset + k + 1])) {
                x = reverse_v_[offset + k + 1];
            } else {
                x = reverse_v_[offset + k - 1] + 1;
            }
            std::int32_t y = x - k;

            const std::int32_t x_start = x;
            const std::int32_t y_start = y;

            while (x < n && y < m && a_[box.x1 + n - 1 - x] == b_[box.y1 + m - 1 - y]) {
                x++;
                y++;
            }

            reverse_v_[offset + k] = x;

            // Check for overlap with forward search (even delta case)
            if (delta % 2 == 0 && k >= delta - d && k <= delta + d) {
                if (forward_v_[offset + delta - k] + reverse_v_[offset + k] >= n) {
                    const std::int32_t f_x = forward_v_[offset + delta - k];
                    const std::int32_t f_y = f_x - (delta - k);

                    return Snake {
                        .x_start = box.x1 + f_x,
                        .y_start = box.y1 + f_y,
                        .x_end = box.x1 + n - x_start,
                        .y_end = box.y1 + m - y_start,
                    };
                }
            }
        }
    }

    std::unreachable();
}

} // namespace renderer
