#include <stdio.h>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "gtest/gtest.h"

#include "test_gv_zeroconf.hpp"

static std::mutex *p_mtx = nullptr;
static std::condition_variable *p_cv = nullptr;

namespace gv = grapevine;

TEST(zeroconf, browser) {
    if (nullptr == p_mtx) {
        p_mtx = new std::mutex();
    }

    if (nullptr == p_cv) {
        p_cv = new std::condition_variable();
    }
    //mtx.lock();
    std::unique_lock<std::mutex> lk(*p_mtx);

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
        printf("Callback worked!\n");
        std::lock_guard<std::mutex> cb_lk(*p_mtx);
        printf("Callback got lock\n");
        //mtx.unlock();
        p_cv->notify_all();
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
    p_cv->wait(lk);
    //mtx.lock();
    //mtx.unlock();
    fprintf(stderr, "test passes\n");
    //std::this_thread::sleep_for(seconds(300));
    fprintf(stderr, "done sleeping\n");
    EXPECT_EQ(1, 1);
}

