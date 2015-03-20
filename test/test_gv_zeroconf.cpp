#include <stdio.h>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "gtest/gtest.h"

#include "test_gv_zeroconf.hpp"

std::mutex mtx;
std::condition_variable cv;

TEST(MyTest, FirstTest) {
    //mtx.lock();
    std::unique_lock<std::mutex> lk(mtx);

    DNSServiceBrowseReply cb = [](
            IN DNSServiceRef service,
            IN DNSServiceFlags flags,
            IN uint32_t interfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN const char *name,
            IN const char *type,
            IN const char *domain,
            IN void *context) -> void
    {
        //std::lock_guard<std::mutex> lk(mtx);
        printf("Callback worked!\n");
        //mtx.unlock();
        cv.notify_all();
        printf("Unlock worked!\n");
    };
    fprintf(stderr, "about to create GV_Browser\n");
    UP_GV_MDNSHandler upMDNSHandler = UP_GV_MDNSHandler(new GV_MDNSHandler());
    upMDNSHandler->setBrowseCallback(cb);
    fprintf(stderr, "about to browse\n");
    upMDNSHandler->enableBrowse();
    fprintf(stderr, "browsing\n");
    using std::chrono::high_resolution_clock;
    using std::chrono::seconds;
    cv.wait(lk);
    //mtx.lock();
    //mtx.unlock();
    fprintf(stderr, "test passes\n");
    //std::this_thread::sleep_for(seconds(300));
    fprintf(stderr, "done sleeping\n");
    EXPECT_EQ(1, 1);
}

int
whatever() {
    printf("icky\n");
    return 0;
}
