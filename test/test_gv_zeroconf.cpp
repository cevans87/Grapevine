#include <stdio.h>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "gtest/gtest.h"

#include "test_gv_zeroconf.h"

//using std::chrono;
using std::condition_variable;
using std::unique_lock;
using std::mutex;
using std::defer_lock;
using std::this_thread::sleep_for;
using std::chrono::seconds;

static mutex *g_pmtxTest = nullptr;
static condition_variable *g_pcvTest = nullptr;
static bool g_bOk = false;

namespace gv = grapevine;

TEST(zeroconf, browser) {
    if (nullptr == g_pmtxTest) {
        g_pmtxTest = new mutex();
    }

    if (nullptr == g_pcvTest) {
        g_pcvTest = new condition_variable();
    }
    unique_lock<mutex> lk(*g_pmtxTest);

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
        unique_lock<mutex> ul(*g_pmtxTest, defer_lock);
        sleep_for(seconds(1));
        if (false == ul.try_lock()) {
            return;
        }
        g_bOk = true;
        g_pcvTest->notify_all();
    };
    //gv::UP_ZeroconfClient upZeroconfClient = gv::UP_ZeroconfClient(new gv::ZeroconfClient());
    gv::UP_ZeroconfClient upZeroconfClient(new gv::ZeroconfClient());
    upZeroconfClient->setBrowseCallback(cb);
    upZeroconfClient->enableBrowse();
    while (false == g_bOk) {
        g_pcvTest->wait(lk);
    }
    EXPECT_EQ(1, 1);
}

