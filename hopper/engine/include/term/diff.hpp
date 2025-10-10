#include <cassert>
#include <cstddef>
#include <format>
#include <utility>
#include <vector>

/* Structures for the Myer's linear space diff implementation
 */
struct Point {
    std::int32_t x, y;

    bool operator==(const Point& other) const = default;
};

template<>
struct std::formatter<Point> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};

struct Box {
    Point from, to;

    Box(Point from, Point to): from(from), to(to) {
        assert(from.x >= 0 && from.y >= 0 && to.x >= from.x && to.y >= from.y);
    }

    Box(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2):
        from({x1, y1}),
        to({x2, y2}) {
        assert(x1 >= 0 && y1 >= 0 && x2 >= x1 && y2 >= y1);
    }

    [[nodiscard]] std::int32_t width() const {
        return to.x - from.x;
    }

    [[nodiscard]] std::int32_t height() const {
        return to.y - from.y;
    }

    [[nodiscard]] std::int32_t size() const {
        return width() + height();
    }
};

template<>
struct std::formatter<Box> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Box& box, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "Box(from: {}, to: {})", box.from, box.to);
    }
};

struct Snake {
    Point from, to;

    bool operator==(const Snake& other) const = default;

    friend std::ostream& operator<<(std::ostream& os, const Snake& snake) {
        return os << std::format("Snake(from: {}, to: {})", snake.from, snake.to);
    }
};

template<>
struct std::formatter<Snake> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Snake& snake, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "Snake(from: {}, to: {})", snake.from, snake.to);
    }
};

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
    // Only insertions (no matches in this region)
    if (box.width() == 0) {
        return;
    }
    // Only deletions (no matches in this region)
    if (box.height() == 0) {
        return;
    }

    Snake snake = find_middle_snake(box);

    // Recursively handle the region before the snake
    Box before_region {box.from, snake.from};
    if (before_region.width() != 0 && before_region.height() != 0) {
        diff_recursive(before_region, trace);
    }

    // Add the snake (matching elements) to the trace
    const std::int32_t snake_length = snake.to.x - snake.from.x;
    for (std::int32_t i = 0; i < snake_length; ++i) {
        trace.emplace_back(snake.from.x + i, snake.from.y + i);
    }

    // Recursively handle the region after the snake
    Box after_region {snake.to, box.to};
    if (after_region.width() != 0 && after_region.height() != 0) {
        diff_recursive(after_region, trace);
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
        return Snake {box.from, box.from};
    }
    if (n == 0) {
        return Snake {box.from, box.from};
    }
    if (m == 0) {
        return Snake {box.from, box.from};
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
            while (x < n && y < m && a_[box.from.x + x] == b_[box.from.y + y]) {
                x++;
                y++;
            }
            forward_v_[offset + k] = x;

            // Check for overlap with reverse search (odd delta case)
            if (delta % 2 != 0 && k >= delta - d + 1 && k <= delta + d - 1) {
                if (forward_v_[offset + k] + reverse_v_[offset + delta - k] >= n) {
                    return Snake {
                        .from = {box.from.x + x_start, box.from.y + y_start},
                        .to = {box.from.x + x, box.from.y + y},
                    };
                }
            }
        }

        // Reverse search
        for (std::int32_t c = -d; c <= d; c += 2) {
            std::int32_t x;

            if (c == -d || (c != d && reverse_v_[offset + c - 1] < reverse_v_[offset + c + 1])) {
                x = reverse_v_[offset + c + 1];
            } else {
                x = reverse_v_[offset + c - 1] + 1;
            }
            std::int32_t y = x - c;

            const std::int32_t x_start = x;
            const std::int32_t y_start = y;

            while (x < n && y < m && a_[box.to.x - 1 - x] == b_[box.to.y - 1 - y]) {
                x++;
                y++;
            }

            reverse_v_[offset + c] = x;

            // Check for overlap with forward search (even delta case)
            if (delta % 2 == 0 && c >= delta - d && c <= delta + d) {
                if (forward_v_[offset + delta - c] + reverse_v_[offset + c] >= n) {
                    return Snake {
                        .from = {box.to.x - x, box.to.y - y},
                        .to = {box.to.x - x_start, box.to.y - y_start},
                    };
                }
            }
        }
    }

    std::unreachable();
}
