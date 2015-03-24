#include <future>
#include <functional>
#include <chrono>

#include "test_gv_channel.hpp"
#include "gtest/gtest.h"

int const g_nItems = 100000;
int const g_nThreadsLo = 2;
int const g_nThreadsMed = 10;
int const g_nThreadsHi = 50;

TEST(channel, cap0) {
    int nItems = g_nItems;
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
    int nItems = g_nItems;
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

TEST(channel, many_consumers) {
    int nItems = g_nItems;
    int expect = ((nItems - 1) * (nItems)) / 2; // sum of 0 to (n - 1)
    int accum = 0;
    GV_Channel<int> *chan = new GV_Channel<int>(1);
    std::vector<std::future<int>> results;

    auto putThread = [chan, nItems] () {

        //printf("Starting producer\n");
        for (int i = 0; i < nItems; ++i) {
            //printf("Putting %i\n", i);
            std::unique_ptr<int> a = std::unique_ptr<int>(new int(i));
            chan->put(&a);
        }
        chan->close();
    };

    auto getThread = [chan, nItems] () {
        GV_ERROR error = GV_ERROR_SUCCESS;
        int ret = 0;
        std::unique_ptr<int> a;

        while (GV_ERROR_SUCCESS == chan->get(&a)) {
            ret += *a;
        }

        return ret;
    };

    std::future<void> fut = std::async(std::launch::async, putThread);

    for (int i = 0; i < g_nThreadsHi; ++i) {
        results.emplace_back(std::async(std::launch::async, getThread));
    }

    for (std::future<int> &result: results) {
        int res = result.get();
        accum += res;
    }

    EXPECT_EQ(accum, expect);
}


TEST(channel, nonempty_on_close) {
    int nItems = 1000;
    int accum = 0;
    int expect = ((nItems - 1) * nItems) / 2;
    GV_Channel<int> *chan = new GV_Channel<int>(1000);
    std::vector<std::future<int>> results;

    for (int i = 0; i < nItems; ++i) {
        std::unique_ptr<int> a = std::unique_ptr<int>(new int(i));
        chan->put(&a);
    }
    chan->close();

    auto getThread = [chan] () {
        GV_ERROR error = GV_ERROR_SUCCESS;
        int ret = 0;
        std::unique_ptr<int> a;

        while (GV_ERROR_SUCCESS == chan->get(&a)) {
            ret += *a;
        }

        return ret;
    };

    for (int i = 0; i < g_nThreadsMed; ++i) {
        results.emplace_back(std::async(std::launch::async, getThread));
    }

    for (std::future<int> &result: results) {
        int res = result.get();
        accum += res;
    }

    EXPECT_EQ(accum, expect);
}

