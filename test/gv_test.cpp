#include "gtest/gtest.h"
#include "gv_test.hpp"

int
main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    printf("Haha\n");
    whatever();
    return RUN_ALL_TESTS();
}
