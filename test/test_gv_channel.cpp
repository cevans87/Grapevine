#include <future>
#include <functional>

#include "test_gv_channel.hpp"
#include "gtest/gtest.h"

TEST(channel, cap0) {
    int nItems = 1000;

    GV_Channel<int> *chan = new GV_Channel<int>(0);

    auto putThread = [chan, nItems] () {

        for (int i = 0; i < nItems; ++i) {
            std::unique_ptr<int> a = std::unique_ptr<int>(new int(i));
            chan->put(&a);
        }
    };

    auto getThread = [chan, nItems] () {
        std::unique_ptr<int> a;

        for (int i = 0; i < nItems; ++i) {
            chan->get(&a);
            EXPECT_EQ(i, *a);
        }
    };


    std::future<void> fut = std::async(std::launch::async, putThread);

    getThread();
}

TEST(channel, cap1) {
    int nItems = 1000;

    GV_Channel<int> *chan = new GV_Channel<int>(1);

    auto putThread = [chan, nItems] () {

        for (int i = 0; i < nItems; ++i) {
            std::unique_ptr<int> a = std::unique_ptr<int>(new int(i));
            chan->put(&a);
        }
    };

    auto getThread = [chan, nItems] () {
        std::unique_ptr<int> a;

        for (int i = 0; i < nItems; ++i) {
            chan->get(&a);
            EXPECT_EQ(i, *a);
        }
    };


    std::future<void> fut = std::async(std::launch::async, putThread);

    getThread();
}
