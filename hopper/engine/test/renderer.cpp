#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "renderer.hpp"

// Test fixture for the MyersDiff algorithm
TEST(MyersDiffTest, SimpleDiff) {
    // Input sequences from the original main() function
    std::vector<char> as { 'A', 'B', 'C', 'A', 'B', 'A' };
    std::vector<char> bs { 'C', 'A', 'B', 'B', 'A' };

    // The expected trace represents the pairs of indices for common elements.
    // The common subsequence is C, A, B, A.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> expected_trace = {
        { 2, 0 }, // C
        { 3, 1 }, // A
        { 4, 2 }, // B
        { 5, 4 } // A
    };

    // Instantiate the differ and run the algorithm
    renderer::MyersDiff<char> differ(as, bs);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> actual_trace = differ.diff();

    // Assert that the calculated trace is equal to the expected trace.
    // std::vector has a built-in operator== that compares element by element.
    ASSERT_EQ(actual_trace, expected_trace);
}
