#ifndef GRAPEVINE_SRC_GV_BROWSER_HPP_
#define GRAPEVINE_SRC_GV_BROWSER_HPP_

#include <dns_sd.h>

#include "gv_type.hpp"

// FIXME cevans87: What should the callback give?
typedef int (*gv_callback)(void);

class GV_Browser
{
    public:
        GV_Browser();
        int enable();
        int disable();
        int register_callback(gv_callback callback);
    private:
        void handleEvents(DNSServiceRef serviceRef);
        // FIXME cevans87: make this non-static.
        static void callback(
                IN DNSServiceRef service,
                IN DNSServiceFlags flags,
                IN uint32_t interfaceIndex,
                IN DNSServiceErrorType errorCode,
                IN const char *name,
                IN const char *type,
                IN const char *domain,
                IN void *context
                );
        DNSServiceErrorType browse();
        DNSServiceRef _serviceRef;
        gv_callback _callback;
};

using UP_GV_Browser = std::unique_ptr<GV_Browser>;

#endif // GRAPEVINE_SRC_GV_BROWSER_HPP_

