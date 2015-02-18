#ifndef GRAPEVINE_SRC_GV_BROWSER_HPP_
#define GRAPEVINE_SRC_GV_BROWSER_HPP_

#include <dns_sd.h>

#include "gv_type.hpp"

// FIXME cevans87: What should the callback give?
using gv_browse_callback = DNSServiceBrowseReply;
using gv_browse_context = void *;

class GV_Browser
{
    public:
        GV_Browser();
        GV_Browser(gv_browse_callback, gv_browse_context);
        GV_ERROR enable();
        int disable();
        int register_callback(gv_browse_callback);
    private:
        void handleEvents(DNSServiceRef serviceRef);
        static void DNSServiceBrowseCallback(
            IN DNSServiceRef service,
            IN DNSServiceFlags flags,
            IN uint32_t interfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN const char *name,
            IN const char *type,
            IN const char *domain,
            IN void *context
            );
        void DNSServiceBrowseCallback(
                IN DNSServiceRef service,
                IN DNSServiceFlags flags,
                IN uint32_t interfaceIndex,
                IN DNSServiceErrorType errorCode,
                IN const char *name,
                IN const char *type,
                IN const char *domain
                );
        DNSServiceErrorType browse();
        DNSServiceRef _serviceRef;
        gv_browse_callback _mCallback;
        gv_browse_context _mContext;
};

using UP_GV_Browser = std::unique_ptr<GV_Browser>;

#endif // GRAPEVINE_SRC_GV_BROWSER_HPP_

