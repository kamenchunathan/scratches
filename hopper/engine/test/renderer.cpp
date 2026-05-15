#include <gtest/gtest.h>
#include <vector>

#include "term/diff.hpp"

template<typename T>
class MyersDiffTest: public MyersDiff<T> {
public:
    MyersDiffTest(const std::vector<T>& a, const std::vector<T>& b): MyersDiff<T>(a, b) {}

    Snake test_find_middle_snake(const Box& box) {
        return this->find_middle_snake(box);
    }
};

TEST(MyersDiffFindMiddleSnakeTest, Identity) {
    std::vector<char> as {'A', 'B', 'C'};
    std::vector<char> bs {'A', 'B', 'C'};

    MyersDiffTest<char> differ(as, bs);

    Box box(0, 0, as.size(), bs.size());
    Snake snake = differ.test_find_middle_snake(box);

    Snake expected_snake = {{0, 0}, {3, 3}};
    EXPECT_EQ(snake, expected_snake);
}

TEST(MyersDiffFindMiddleSnakeTest, SimpleEvenDelta) {
    std::vector<char> as {'A', 'B', 'C', 'A', 'B', 'A'};
    std::vector<char> bs {'C', 'B', 'A', 'B', 'C'};

    MyersDiffTest<char> differ(as, bs);

    Box box(0, 0, as.size(), bs.size());
    Snake snake = differ.test_find_middle_snake(box);

    Snake expected_snake = {{4, 5}, {4, 5}};

    EXPECT_EQ(snake, expected_snake);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
