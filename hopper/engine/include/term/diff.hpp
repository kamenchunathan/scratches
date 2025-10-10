#include <cassert>
#include <cstddef>
#include <format>
#include <string_view>
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

enum class EditTag {
    Match,
    Delete,
    Insert,

};

template<>
struct std::formatter<EditTag> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const EditTag& tag, std::format_context& ctx) const {
        std::string_view tag_str;
        switch (tag) {
            break;
            case EditTag::Match:
                tag_str = "Match";
                break;

            case EditTag::Delete:
                tag_str = "Delete";
                break;

            case EditTag::Insert:
                tag_str = "Insert";
                break;
        }
        return std::format_to(ctx.out(), "EditTag::{}", tag_str);
    }
};

inline std::ostream& operator<<(std::ostream& os, const EditTag& edit) {
    return os << std::format("{}", edit);
}

struct Edit {
    EditTag tag;
    std::int32_t a_index;
    std::int32_t b_index;
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

    void build_trace(const Box& box, std::vector<Snake>& out);

private:
    Snake find_middle_snake(const Box&);

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
void MyersDiff<T>::build_trace(const Box& box, std::vector<Snake>& trace) {
    const std::int32_t N = box.to.x - box.from.x;
    const std::int32_t M = box.to.y - box.from.y;

    // Base case: if one side is empty, no more snakes to find
    if (N == 0 || M == 0)
        return;

    // Find the middle snake
    Snake mid = find_middle_snake(box);

    // If the snake covers the whole box or is degenerate, stop recursion
    if ((mid.from == box.from && mid.to == box.to) || (mid.from == mid.to)) {
        trace.push_back(mid);
        return;
    }

    // Recurse on left part (before the snake)
    if (mid.from.x > box.from.x || mid.from.y > box.from.y) {
        Box left(box.from, mid.from);
        build_trace(left, trace);
    }

    // Add the middle snake itself
    trace.push_back(mid);

    // Recurse on right part (after the snake)
    if (mid.to.x < box.to.x || mid.to.y < box.to.y) {
        Box right(mid.to, box.to);
        build_trace(right, trace);
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
