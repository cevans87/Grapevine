#include <stdio.h>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "arpa/inet.h"
#include "gtest/gtest.h"

#include "gv_util.h"
#include "gv_zeroconf.h"

// TODO test name conflicts when implemented.
// TODO massive stress tests needed.
// TODO multi-process tests needed. Unfortunately, that'll be way easier to
// coordinate with a fully-functioning Communicator.

using std::condition_variable;
using std::unique_lock;
using std::lock_guard;
using std::mutex;
using std::defer_lock;
using std::this_thread::sleep_for;
using std::chrono::seconds;

static mutex *g_pmtxTest = nullptr;
static condition_variable *g_pcvTest = nullptr;
static bool g_bOk[2] = {false, false};

namespace gv = grapevine;

using std::make_unique;

TEST(zeroconf, browser) {
    g_bOk[0] = false;
    g_bOk[1] = false;
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
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN uint32_t uInterfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszRegType,
            IN char const *pszDomainName,
            IN void *context) -> void
#pragma clang diagnostic pop
    {
        unique_lock<mutex> ul(*g_pmtxTest, defer_lock);
        sleep_for(seconds(1));
        if (false == ul.try_lock()) {
            return;
        }
        g_bOk[0] = true;
        g_pcvTest->notify_all();
    };
    gv::UPZeroconfClient upZeroconfClient = make_unique<gv::ZeroconfClient>();
    upZeroconfClient->set_browse_callback(cb);
    upZeroconfClient->enable_browse();
    while (false == g_bOk[0]) {
        g_pcvTest->wait(lk);
    }
    EXPECT_EQ(1, 1);
}

TEST(zeroconf, RegisterAndResolve) {
    g_bOk[0] = false;
    g_bOk[1] = false;
    if (nullptr == g_pmtxTest) {
        g_pmtxTest = new mutex();
    }

    if (nullptr == g_pcvTest) {
        g_pcvTest = new condition_variable();
    }
    unique_lock<mutex> lk(*g_pmtxTest);

    DNSServiceRegisterReply register_cb = [](
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszRegType,
            IN char const *pszDomainName,
            IN void *context) -> void
#pragma clang diagnostic pop
    {
        lock_guard<mutex> lg(*g_pmtxTest);
        g_bOk[0] = true;
    };

    DNSServiceResolveReply resolve_cb = [](
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN uint32_t uInterfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszHostName,
            IN uint16_t uPort,
            IN uint16_t uTxtLen,
            IN unsigned char const *pszTxtRecord,
            IN void *context) -> void
#pragma clang diagnostic pop
    {
        lock_guard<mutex> lg(*g_pmtxTest);
        g_bOk[1] = true;
        g_pcvTest->notify_all();
    };
    gv::UPZeroconfClient upZeroconfClient = make_unique<gv::ZeroconfClient>();
    upZeroconfClient->add_register_callback(
            0, // flags
            0, // uInterfaceIndex,
            nullptr, // pszDomainName,
            nullptr, // pszHostName,
            "happy", // pszServiceName,
            50001, // uPortNum,
            nullptr, // pTxtRecord
            0, // uTxtLen,
            register_cb,
            nullptr); // callback
    upZeroconfClient->add_resolve_callback(
            "happy",
            resolve_cb,
            nullptr);
    while (false == g_bOk[1]) {
        g_pcvTest->wait(lk);
    }
    EXPECT_EQ(1, 1);
}

