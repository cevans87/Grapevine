#include <stdio.h>
#include "gtest/gtest.h"

#include "test_gv_browser.hpp"

TEST(MyTest, FirstTest) {
    printf("Haha\n");
    EXPECT_EQ(1, 1);
}

int
whatever() {
    printf("icky\n");
    return 0;
}
