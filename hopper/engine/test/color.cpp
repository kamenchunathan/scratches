#include <gtest/gtest.h>

#include "color.hpp"

TEST(MyersDiff, BuildTrace) {
    core::ColorRGB8 a {0, 0, 0};
    core::ColorRGB8 b {0, 0, 0};

    EXPECT_EQ(a, b);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
