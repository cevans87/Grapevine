#ifndef GRAPEVINE_TEST_GV_TEST_HPP_
#define GRAPEVINE_TEST_GV_TEST_HPP_

#include "gv_type.hpp"
#include "test_gv_channel.hpp"
//#include "test_gv_zeroconf.hpp"
//#include "test_gv_browser.hpp"

/*
#define GV_TEST_INIT() int success = 0; int tests = 0; bool passed_test;

#define GV_TEST_GT >
#define GV_TEST_GTE >=
#define GV_TEST_EQ ==

#define GV_TEST_FAIL_PRINT(fmt, ...)                                        \
    do                                                                      \
    {                                                                       \
        fprintf(stderr, "GV_TEST: %s:%d:%s(): Failed: ", fmt "\n",          \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__);               \
    } while (0)

#define GV_EXPECT(a, b, expect, sym, hint_text)                             \
    do                                                                      \
    {                                                                       \
        tests++;                                                            \
        if (expect == (a sym b))                                            \
        {                                                                   \
            passed_test = true;                                             \
            success++;                                                      \
        }                                                                   \
        else                                                                \
        {                                                                   \
            pass = false;                                                   \
            GV_TEST_FAIL_PRINT(hint_text);                                  \
        }                                                                   \
    } while (0)

#define GV_EXPECT_GT(a, b, hint_text)                                       \
    GV_EXPECT(a, b, true, GV_TEST_GT, hint_text)

#define GV_EXPECT_LT(a, b, hint_text)                                       \
    GV_EXPECT(b, a, true, GV_TEST_GT, hint_text)

#define GV_EXPECT_GEQ(a, b, hint_text)                                      \
    GV_EXPECT(a, b, true, GV_TEST_GTE, hint_text)

#define GV_EXPECT_LEQ(a, b, hint_text)                                      \
    GV_EXPECT(b, a, true, GV_TEST_GTE, hint_text)

#define GV_EXPECT_EQ(a, b, hint_text)                                       \
    GV_EXPECT(a, b, true, GV_TEST_EQ, hint_text)

#define GV_EXPECT_NEQ(a, b, hint_text)                                      \
    GV_EXPECT(a, b, false, hint_text)

#define GV_EXPECT_TRUE(a, hint_text)                                        \
    GV_EXPECT_EQ(a, true, hint_text)

#define GV_EXPECT_FALSE(a, hint_text)                                       \
    GV_EXPECT_EQ(a, false, hint_text)

#define GV_ASSERT_EQ(a, b, hint_text)                                       \
    GV_EXPECT_EQ(a, b, hint_text);                                          \
    do                                                                      \
    {                                                                       \
        if (false = passed_test)                                            \
        {                                                                   \
            GV_TEST_FAIL_PRINT(hint_text);                                  \
            fprintf("Aborting remaining tests\n");                          \
        }                                                                   \
    } while (0)

*/
#endif // GRAPEVINE_TEST_HPP_GV_TEST_HPP_

