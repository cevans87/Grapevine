#ifndef GRAPEVINE_SRC_GV_ZEROCONF_HPP_
#define GRAPEVINE_SRC_GV_ZEROCONF_HPP_

#include <dns_sd.h>
#include <functional>
#include <future>

#include "gv_type.hpp"
#include "gv_channel.hpp"

using gv_browse_callback = DNSServiceBrowseReply;
using gv_resolve_callback = DNSServiceResolveReply;
using gv_register_callback = DNSServiceRegisterReply;

using gv_browse_context = void *;

class GV_MDNSHandler
{
    public:
        GV_ERROR setBrowseCallback(
            IN DNSServiceBrowseReply callback);
        GV_ERROR enableBrowse();
        GV_ERROR disableBrowse();

        GV_ERROR setResolveCallback(
            IN char *pszServiceName,
            IN std::function<void()> callback);
        GV_ERROR enableResolve(
            IN char *pszServiceName);
        GV_ERROR disableResolve(
            IN char *pszServiceName);

        GV_ERROR setRegisterCallback(
            IN char *pszServiceName,
            IN std::function<void()> callback);
        GV_ERROR enableRegister(
            IN char *pszServiceName);
        GV_ERROR disableRegister(
            IN char *pszServiceName);

    private:
        gv_browse_callback _mBrowseCallback;
        std::future<void> _mFutureHandleEvents; // handleEvents
        DNSServiceRef _mServiceRef; // FIXME get rid of this. We need more than one.

        static void handleEvents(
            IN DNSServiceRef serviceRef);
        static void browseCallback(
            IN DNSServiceRef service,
            IN DNSServiceFlags flags,
            IN uint32_t interfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN const char *name,
            IN const char *type,
            IN const char *domain,
            IN void *context);
};

//class GV_Browser
//{
//    public:
//        GV_Browser();
//        GV_Browser(gv_browse_callback, gv_browse_context);
//        GV_ERROR enable();
//        int disable();
//        int register_callback(gv_browse_callback);
//    private:
//        void handleEvents(DNSServiceRef serviceRef);
//        static void DNSServiceBrowseCallback(
//                IN DNSServiceRef service,
//                IN DNSServiceFlags flags,
//                IN uint32_t interfaceIndex,
//                IN DNSServiceErrorType errorCode,
//                IN const char *name,
//                IN const char *type,
//                IN const char *domain,
//                IN void *context
//                );
//        void DNSServiceBrowseCallback(
//                IN DNSServiceRef service,
//                IN DNSServiceFlags flags,
//                IN uint32_t interfaceIndex,
//                IN DNSServiceErrorType errorCode,
//                IN const char *name,
//                IN const char *type,
//                IN const char *domain
//                );
//        DNSServiceErrorType browse();
//        DNSServiceRef _serviceRef;
//        gv_browse_callback _mCallback;
//        gv_browse_context _mContext;
//};

using UP_GV_MDNSHandler = std::unique_ptr<GV_MDNSHandler>;

#endif // GRAPEVINE_SRC_GV_ZEROCONF_HPP_

