#include <gtest/gtest.h>
#include <vector>

#include "term/diff.hpp"

TEST(MyersDiffFindMiddleSnakeTest, Identity) {
    std::vector<char> as {'A', 'B', 'C'};
    std::vector<char> bs {'A', 'B', 'C'};

    MyersDiff<char> differ(as, bs);

    Box box(0, 0, as.size(), bs.size());
    Snake snake = differ.find_middle_snake(box);

    Snake expected_snake = {{0, 0}, {3, 3}};
    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, SimpleEvenDelta) {
    std::vector<char> as {'A', 'B', 'C', 'A', 'B', 'A'};
    std::vector<char> bs {'C', 'B', 'A', 'B', 'C'};

    MyersDiff<char> differ(as, bs);

    Box box(0, 0, as.size(), bs.size());
    Snake snake = differ.find_middle_snake(box);

    Snake expected_snake = {{4, 5}, {4, 5}};

    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, SimpleOddDelta) {
    std::vector<char> as {'A', 'B'};
    std::vector<char> bs {'A', 'C', 'B'};

    MyersDiff<char> differ(as, bs);

    Box box(0, 0, as.size(), bs.size());
    Snake snake = differ.find_middle_snake(box);

    // The middle snake should be 'B'.
    // It starts at a[1], b[2] and has length 1.
    Snake expected_snake = {{1, 2}, {2, 3}};
    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, OffsetBoxIdentity) {
    std::vector<char> as {'X', 'Y', 'Z', 'A', 'B', 'C'};
    std::vector<char> bs {'P', 'Q', 'R', 'A', 'B', 'C'};

    MyersDiff<char> differ(as, bs);

    Box box(3, 3, 6, 6);
    Snake snake = differ.find_middle_snake(box);

    Snake expected_snake = {{3, 3}, {6, 6}};
    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, OffsetBoxWithDifferences) {
    std::vector<char> as {'X', 'Y', 'Z', 'A', 'B', 'C', 'D', 'E'};
    std::vector<char> bs {'P', 'Q', 'R', 'A', 'X', 'C', 'D', 'E'};

    MyersDiff<char> differ(as, bs);

    Box box(3, 3, 8, 8);
    Snake snake = differ.find_middle_snake(box);

    EXPECT_GE(snake.from.x, 3);
    EXPECT_GE(snake.from.y, 3);
    EXPECT_LE(snake.to.x, 8);
    EXPECT_LE(snake.to.y, 8);
}

TEST(MyersDiffFindMiddleSnakeTest, OffsetBoxEvenDelta) {
    std::vector<char> as {'X', 'Y', 'A', 'B', 'C', 'A', 'B', 'A'};
    std::vector<char> bs {'P', 'Q', 'C', 'B', 'A', 'B', 'C'};

    MyersDiff<char> differ(as, bs);

    // Test subregion: as[2..8] = "ABCABA", bs[2..7] = "CBABC"
    Box box(2, 2, 8, 7);
    Snake snake = differ.find_middle_snake(box);

    // Verify snake is within bounds
    EXPECT_GE(snake.from.x, 2);
    EXPECT_GE(snake.from.y, 2);
    EXPECT_LE(snake.to.x, 8);
    EXPECT_LE(snake.to.y, 7);
}

TEST(MyersDiffFindMiddleSnakeTest, OffsetBoxOddDelta) {
    std::vector<char> as {'X', 'Y', 'Z', 'A', 'B'};
    std::vector<char> bs {'P', 'Q', 'R', 'A', 'C', 'B'};

    MyersDiff<char> differ(as, bs);

    // Test subregion: as[3..5] = "AB", bs[3..6] = "ACB"
    Box box(3, 3, 5, 6);
    Snake snake = differ.find_middle_snake(box);

    // Should find 'B' at the end
    Snake expected_snake = {{4, 5}, {5, 6}};
    EXPECT_EQ(snake, expected_snake);
}

// Test full diff with offset scenarios
TEST(MyersDiffTest, FullDiffIdentical) {
    std::vector<char> as {'A', 'B', 'C'};
    std::vector<char> bs {'A', 'B', 'C'};

    MyersDiff<char> differ(as, bs);
    auto result = differ.diff();

    // All elements should match
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], std::make_pair(0u, 0u));
    EXPECT_EQ(result[1], std::make_pair(1u, 1u));
    EXPECT_EQ(result[2], std::make_pair(2u, 2u));
}

TEST(MyersDiffTest, FullDiffInsertion) {
    std::vector<char> as {'A', 'C'};
    std::vector<char> bs {'A', 'B', 'C'};

    MyersDiff<char> differ(as, bs);
    auto result = differ.diff();

    // 'A' and 'C' should match
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], std::make_pair(0u, 0u)); // 'A' matches
    EXPECT_EQ(result[1], std::make_pair(1u, 2u)); // 'C' matches (B was inserted)
}

TEST(MyersDiffTest, FullDiffDeletion) {
    std::vector<char> as {'A', 'B', 'C'};
    std::vector<char> bs {'A', 'C'};

    MyersDiff<char> differ(as, bs);
    auto result = differ.diff();

    // 'A' and 'C' should match
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], std::make_pair(0u, 0u)); // 'A' matches
    EXPECT_EQ(result[1], std::make_pair(2u, 1u)); // 'C' matches (B was deleted)
}

TEST(MyersDiffTest, FullDiffComplex) {
    std::vector<char> as {'A', 'B', 'C', 'A', 'B', 'B', 'A'};
    std::vector<char> bs {'C', 'B', 'A', 'B', 'A', 'C'};

    MyersDiff<char> differ(as, bs);
    auto result = differ.diff();

    // Verify result is non-empty and reasonable
    EXPECT_GT(result.size(), 0);

    // All matched pairs should be valid indices
    for (const auto& [a_idx, b_idx]: result) {
        EXPECT_LT(a_idx, as.size());
        EXPECT_LT(b_idx, bs.size());
        EXPECT_EQ(as[a_idx], bs[b_idx]);
    }
}

TEST(MyersDiffTest, EmptySequences) {
    std::vector<char> as;
    std::vector<char> bs;

    MyersDiff<char> differ(as, bs);
    auto result = differ.diff();

    EXPECT_EQ(result.size(), 0);
}

TEST(MyersDiffTest, OneEmptySequence) {
    std::vector<char> as {'A', 'B', 'C'};
    std::vector<char> bs;

    MyersDiff<char> differ(as, bs);
    auto result = differ.diff();

    // No matches possible
    EXPECT_EQ(result.size(), 0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
