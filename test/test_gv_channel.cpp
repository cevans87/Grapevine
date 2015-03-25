#include <future>
#include <functional>
#include <chrono>

#include "test_gv_channel.hpp"
#include "gtest/gtest.h"

int const g_nItems = 100000;
//int const g_nThreadsLo = 2;
int const g_nThreadsMed = 10;
int const g_nThreadsHi = 50;

namespace gv = grapevine;

// Puts items from 0 to (nItems - 1) into the channel in order.
// IN *chan - channel to 'put' items into.
// IN nItems - number if ints to put into channel from 0 to (nItems - 1)
// IN bClose - close channel after finished?
static void
generic_putter(
    IN gv::Channel<int> *chan,
    IN int nItems,
    IN bool bClose
    )
{
    for (int i = 0; i < nItems; ++i) {
        std::unique_ptr<int> a = std::unique_ptr<int>(new int(i));
        chan->put(&a);
    }

    if (bClose) {
        chan->close();
    }
}

// Gets ints from the channel until it closes and return the accumulated value.
// IN *chan - Channel to get ints from until closed.
// Returns sum of all ints gotten from channel.
static int
accumulator_getter(
    IN gv::Channel<int> *chan
    )
{
    int ret = 0;
    std::unique_ptr<int> a;

    while (GV_ERROR_SUCCESS == chan->get(&a)) {
        ret += *a;
    }

    return ret;
}

// Gets ints from the channel until it closes and return the accumulated value.
// IN *chan - Channel to get ints from until closed.
// Returns sum of all ints gotten from channel.
static int
nowait_getter(
    IN gv::Channel<int> *chan
    )
{
    GV_ERROR error;
    int ret = 0;
    std::unique_ptr<int> a;

    do {
        error = chan->get_nowait(&a);
        if (GV_ERROR_SUCCESS == error) {
            ret += *a;
        }
    } while (GV_ERROR_CHANNEL_CLOSED != error);

    return ret;
}

// Puts items from 0 to (nItems - 1) into the channel in order using
// put_nowait. Repeatedly tries each item until they succeeed.
// IN *chan - channel to 'put' items into.
// IN nItems - number if ints to put into channel from 0 to (nItems - 1)
// IN bClose - close channel after finished?
static void
nowait_putter(
    IN gv::Channel<int> *chan,
    IN int nItems,
    IN bool bClose
    )
{
    GV_ERROR error;
    for (int i = 0; i < nItems; ++i) {
        std::unique_ptr<int> a = std::unique_ptr<int>(new int(i));
        do {
            error = chan->put_nowait(&a);
        } while (GV_ERROR_SUCCESS != error);
    }

    if (bClose) {
        chan->close();
    }
}

// Tests basic 'put' and 'get' functionality of channel.
// IN capacity - capacity of channel.
static void
capacity_test(
    IN unsigned int capacity
    )
{
    int nItems = g_nItems;
    gv::Channel<int> *chan = new gv::Channel<int>(capacity);

    // closure to validate items coming out of a channel.
    std::function<void(void)> custom_getter = [chan, nItems] () {
        std::unique_ptr<int> a;

        for (int i = 0; i < nItems; ++i) {
            chan->get(&a);
            EXPECT_EQ(i, *a);
        }
    };

    // Start putter thread.
    std::future<void> fut = std::async(std::launch::async,
            generic_putter, chan, nItems, false);

    // Validate items coming out of channel.
    custom_getter();
}

// Tests 'get_nowait'.
static void
nowait_getter_capacity_test(
    IN unsigned int capacity,
    IN unsigned int nPutters,
    IN int nItems
    )
{
    int accum = 0;
    int expect = ((nItems - 1) * nItems) / 2; // Sum of 0 to (n - 1)
    gv::Channel<int> *chan = new gv::Channel<int>(capacity);
    std::vector<std::future<void>> putter_futures;
    std::vector<std::future<int>> getter_futures;

    for (unsigned int i = 0; i < nPutters; ++i) {
        putter_futures.emplace_back(std::async(std::launch::async,
                generic_putter, chan, nItems, false));
    }

    for (int i = 0; i < g_nThreadsMed; ++i) {
        getter_futures.emplace_back(std::async(std::launch::async,
                nowait_getter, chan));
    }

    for (std::future<void> &fut: putter_futures) {
        fut.get();
    }

    chan->close();

    for (std::future<int> &fut: getter_futures) {
        accum += fut.get();
    }

    EXPECT_EQ(accum, expect * static_cast<int>(nPutters));
}

// Tests 'put_nowait'.
static void
nowait_putter_capacity_test(
    IN unsigned int capacity,
    IN unsigned int nGetters,
    IN int nItems
    )
{
    int accum = 0;
    int expect = ((nItems - 1) * nItems) / 2; // Sum of 0 to (n - 1)
    gv::Channel<int> *chan = new gv::Channel<int>(capacity);
    std::vector<std::future<void>> putter_futures;
    std::vector<std::future<int>> getter_futures;

    for (unsigned int i = 0; i < g_nThreadsMed; ++i) {
        putter_futures.emplace_back(std::async(std::launch::async,
                nowait_putter, chan, nItems, false));
    }

    for (unsigned int i = 0; i < nGetters; ++i) {
        getter_futures.emplace_back(std::async(std::launch::async,
                accumulator_getter, chan));
    }

    for (std::future<void> &fut: putter_futures) {
        fut.get();
    }

    chan->close();

    for (std::future<int> &fut: getter_futures) {
        accum += fut.get();
    }

    EXPECT_EQ(accum, expect * g_nThreadsMed);
}

// Sends items through a channel of capacity 0.
TEST(channel, capacity_0) {
    capacity_test(0);
}

// Sends items through a channel of capacity 1.
TEST(channel, capacity_1) {
    capacity_test(1);
}

// Tests channel with high 'get' contention.
TEST(channel, many_getters) {
    int nItems = g_nItems;
    int expect = ((nItems - 1) * nItems) / 2; // Sum of 0 to (n - 1)
    int accum = 0;
    gv::Channel<int> *chan = new gv::Channel<int>(1);
    std::vector<std::future<int>> getter_futures;

    // Start putter thread. Sum of all values it will put equals 'expect'
    // variable.
    std::future<void> putter_future = std::async(std::launch::async,
            generic_putter, chan, nItems, true);

    // Start many getters to contend for channel items.
    for (int i = 0; i < g_nThreadsHi; ++i) {
        getter_futures.emplace_back(std::async(std::launch::async,
                accumulator_getter, chan));
    }

    // Sum values from all getter threads.
    for (std::future<int> &fut: getter_futures) {
        int res = fut.get();
        accum += res;
    }

    EXPECT_EQ(accum, expect);
}

// Tests that get_nowait works on capacity 0 channel. Probably slow.
TEST(channel, nowait_getters_capacity_0_one_tenth_items) {
    nowait_getter_capacity_test(0, g_nThreadsMed, (g_nItems / 10) / g_nThreadsMed);
}

// Tests get_nowait on higher capacity channel, probably much faster than 0
// capacity channel.
TEST(channel, nowait_getters_capacity_100) {
    nowait_getter_capacity_test(100, g_nThreadsMed, g_nItems / g_nThreadsMed);
}

// Tests that put_nowait works on capacity 0 channel. Probably slow.
TEST(channel, nowait_putters_capacity_0_one_tenth_items) {
    nowait_putter_capacity_test(0, g_nThreadsMed, (g_nItems / 10) / g_nThreadsMed);
}

// Tests put_nowait on higher capacity channel, probably much faster than 0
// capacity channel.
TEST(channel, nowait_putters_capacity_100) {
    nowait_putter_capacity_test(100, g_nThreadsMed, g_nItems / g_nThreadsMed);
}

// Tests channel with high 'put' contention.
TEST(channel, med_putters) {
    int nItems = g_nItems / g_nThreadsMed;
    int expect = ((nItems - 1) * nItems) / 2; // Sum of 0 to (n - 1)
    int accum = 0;
    gv::Channel<int> *chan = new gv::Channel<int>(1);
    std::vector<std::future<void>> putter_futures;
    std::vector<std::future<int>> getter_futures;

    // Start putter thread. Sum of all values it will put equals 'expect'
    // variable.
    for (int i = 0; i < g_nThreadsMed; ++i) {
        putter_futures.emplace_back(std::async(std::launch::async,
                generic_putter, chan, nItems, false));
    }

    // Start single getter.
    std::future<int> getter_future = std::async(std::launch::async,
            accumulator_getter, chan);

    // Make sure each putter finishes.
    for (std::future<void> &done: putter_futures) {
        done.get();
    }
    // Close the channel so the getter knows to finish.
    chan->close();

    accum = getter_future.get();

    EXPECT_EQ(accum, g_nThreadsMed * expect);
}

// Makes sure items are retreivable from a closed channel.
TEST(channel, empty_after_close) {
    int nItems = 1000;
    int accum = 0;
    int expect = ((nItems - 1) * nItems) / 2;
    gv::Channel<int> *chan = new gv::Channel<int>(1000);
    std::vector<std::future<int>> getter_futures;

    // Fill the channel.
    generic_putter(chan, nItems, true);

    // Start a few threads to get items from the channel.
    for (int i = 0; i < g_nThreadsMed; ++i) {
        getter_futures.emplace_back(std::async(std::launch::async,
                accumulator_getter, chan));
    }

    // Sum results from all getters.
    for (std::future<int> &fut: getter_futures) {
        int res = fut.get();
        accum += res;
    }

    EXPECT_EQ(accum, expect);
}

