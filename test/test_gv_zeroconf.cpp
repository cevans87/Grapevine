#include <stdio.h>
#include <chrono>
#include <functional>
#include <thread>
#include "gtest/gtest.h"

#include "test_gv_zeroconf.hpp"

TEST(MyTest, FirstTest) {
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
        printf("Callback worked!\n");
    };
    fprintf(stderr, "about to create GV_Browser\n");
    UP_GV_MDNSHandler upMDNSHandler = UP_GV_MDNSHandler(new GV_MDNSHandler());
    fprintf(stderr, "about to browse\n");
    upMDNSHandler->setBrowseCallback(cb);
    upMDNSHandler->enableBrowse();
    using std::chrono::high_resolution_clock;
    using std::chrono::seconds;
    fprintf(stderr, "about to sleep\n");
    std::this_thread::sleep_for(seconds(300));
    fprintf(stderr, "done sleeping\n");
    EXPECT_EQ(1, 1);
}

int
whatever() {
    printf("icky\n");
    return 0;
}
