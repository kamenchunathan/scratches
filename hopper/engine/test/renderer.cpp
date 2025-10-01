#include <gtest/gtest.h>
#include <vector>

#include "renderer/present/term.hpp"

TEST(MyersDiffFindMiddleSnakeTest, Identity) {
    std::vector<char> as { 'A', 'B', 'C' };
    std::vector<char> bs { 'A', 'B', 'C' };

    renderer::MyersDiff<char> differ(as, bs);

    renderer::Box box(0, 0, as.size(), bs.size());
    renderer::Snake snake = differ.find_middle_snake(box);

    // For identical sequences, the middle snake should span the entire range.
    // Note: A failing test here may indicate a bug in the even delta case of
    // the find_middle_snake implementation.
    renderer::Snake expected_snake = { 0, 0, 3, 3 };
    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, SimpleEvenDelta) {
    std::vector<char> as { 'A', 'X', 'B' };
    std::vector<char> bs { 'A', 'Y', 'B' };

    renderer::MyersDiff<char> differ(as, bs);

    renderer::Box box(0, 0, as.size(), bs.size());
    renderer::Snake snake = differ.find_middle_snake(box);

    // The algorithm should find one of the common snakes, 'A' or 'B'.
    // Let's assume it finds 'B'.
    // The snake for 'B' starts at a[2], b[2] and has length 1.
    renderer::Snake expected_snake = { 2, 2, 3, 3 };

    // This test is written for the logically correct output. A failure might
    // indicate the implementation returns a meeting point instead of a snake.
    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, SimpleOddDelta) {
    std::vector<char> as { 'A', 'B' };
    std::vector<char> bs { 'A', 'C', 'B' };

    renderer::MyersDiff<char> differ(as, bs);

    renderer::Box box(0, 0, as.size(), bs.size());
    renderer::Snake snake = differ.find_middle_snake(box);

    // The middle snake should be 'B'.
    // It starts at a[1], b[2] and has length 1.
    renderer::Snake expected_snake = { 1, 2, 2, 3 };
    EXPECT_EQ(snake, expected_snake);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
