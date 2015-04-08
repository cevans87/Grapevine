#include <stdio.h>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "gtest/gtest.h"

#include "test_gv_zeroconf.hpp"

static std::mutex *g_pmtxTest = nullptr;
static std::condition_variable *g_pcvTest = nullptr;
static bool g_bOk = false;

namespace gv = grapevine;

TEST(zeroconf, browser) {
    if (nullptr == g_pmtxTest) {
        g_pmtxTest = new std::mutex();
    }

    if (nullptr == g_pcvTest) {
        g_pcvTest = new std::condition_variable();
    }
    std::unique_lock<std::mutex> lk(*g_pmtxTest);

    DNSServiceBrowseReply cb = [](
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
            IN DNSServiceRef service,
            IN DNSServiceFlags flags,
            IN uint32_t interfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN const char *name,
            IN const char *type,
            IN const char *domain,
            IN void *context) -> void
#pragma clang diagnostic pop
    {
        printf("Callback worked! Got iface idx %u, type %s\n", interfaceIndex, type);
        //using std::chrono::high_resolution_clock;
        //using std::chrono::seconds;
        //std::this_thread::sleep_for(seconds(6));
        printf("callback slept for 6 sec\n");
        //if (false == g_pmtxTest->try_lock()) {
        //    printf("Lock failed in callback\n");
        //    return;
        //}
        g_bOk = true;
        printf("Callback got lock\n");
        //mtx.unlock();
        // FIXME this doesn't always trigger the main function to wake up. Big bug.
        g_pcvTest->notify_all();
        printf("Unlock worked!\n");
    };
    fprintf(stderr, "about to create GV_Browser\n");
    gv::UP_ZeroconfClient upZeroconfClient = gv::UP_ZeroconfClient(new gv::ZeroconfClient());
    upZeroconfClient->setBrowseCallback(cb);
    fprintf(stderr, "about to browse\n");
    upZeroconfClient->enableBrowse();
    fprintf(stderr, "browsing\n");
    using std::chrono::high_resolution_clock;
    using std::chrono::seconds;
    while (false == g_bOk) {
        printf("waiting on ok\n");
        g_pcvTest->wait(lk);
    }
    //lk.unlock();
    //mtx.lock();
    //mtx.unlock();
    fprintf(stderr, "test passes\n");
    //std::this_thread::sleep_for(seconds(300));
    fprintf(stderr, "done sleeping\n");
    EXPECT_EQ(1, 1);
}

