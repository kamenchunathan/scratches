#include <gtest/gtest.h>
#include <print>

#include "term/diff.hpp"

template<typename T>
class MyersDiffTest: public MyersDiff<T> {
public:
    MyersDiffTest(const std::vector<T>& a, const std::vector<T>& b): MyersDiff<T>(a, b) {}

    void test_build_trace(const Box& box, std::vector<Snake>& out) {
        this->build_trace(box, out);
    }
};

TEST(MyersDiff, BuildTrace) {
    std::vector<char> a = {'a', 'b', 'c', 'e', 'h', 'j', 'l', 'm', 'n', 'p'};
    std::vector<char> b = {'b', 'c', 'd', 'e', 'f', 'j', 'k', 'l', 'm', 'r', 's', 't'};
    MyersDiffTest<char> diff(a, b);

    std::vector<Snake> trace;
    diff.test_build_trace(Box(0, 0, a.size(), b.size()), trace);

    for (auto snake: trace) {
        std::println("{}", snake);
    }

    std::vector<Snake> expected = {
        {{1, 0}, {3, 2}},
        {{3, 3}, {4, 4}},
        {{5, 5}, {6, 6}},
        {{6, 7}, {8, 9}},
    };

    ASSERT_EQ(trace.size(), expected.size());
    for (size_t i = 0; i < trace.size(); ++i) {
        EXPECT_EQ(trace[i].from, expected[i].from);
        EXPECT_EQ(trace[i].to, expected[i].to);
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
