#ifndef GRAPEVINE_SRC_GV_ZEROCONF_H_
#define GRAPEVINE_SRC_GV_ZEROCONF_H_

#include <functional>
#include <future>
#include <string>

#include <dns_sd.h>

#include "gv_type.h"
#include "gv_channel.hpp"

namespace grapevine {

using gv_browse_callback = DNSServiceBrowseReply;
using gv_resolve_callback = DNSServiceResolveReply;
using gv_register_callback = DNSServiceRegisterReply;

struct UPServiceRefDeleter {
    void operator()(DNSServiceRef *serviceRef) {
        DNSServiceRefDeallocate(*serviceRef);
        delete serviceRef;
    }
};

using UPServiceRef = std::unique_ptr<DNSServiceRef, UPServiceRefDeleter>;
using UPCHServiceRef = UPChannel<DNSServiceRef, UPServiceRefDeleter>;

class ZeroconfClient
{
    public:
        ZeroconfClient();
        ~ZeroconfClient();

        // Be aware that the browse callback may be called again very quickly,
        // possibly while the previous call is still running. The callback
        // should be prepared for this.
        GV_ERROR setBrowseCallback(
            IN DNSServiceBrowseReply callback);
        GV_ERROR enableBrowse();
        GV_ERROR disableBrowse();

        GV_ERROR addResolveCallback(
            IN char const *pszServiceName,
            IN gv_resolve_callback callback);
        GV_ERROR enableResolve(
            IN char *pszServiceName);
        GV_ERROR disableResolve(
            IN char *pszServiceName);

        GV_ERROR addRegisterCallback(
            IN char const *pszServiceName,
            IN gv_register_callback callback);
        GV_ERROR enableRegister(
            IN char *pszServiceName);
        GV_ERROR disableRegister(
            IN char *pszServiceName);

    private:
        unsigned int const _ukChannelSize = 1;
        // FIXME place these things in channels to make the ZeroconfClient
        // thread safe? That'd be pretty much every member in a channel.
        UPCHServiceRef _upchAddServiceRef;
        UPCHServiceRef _upchRemoveServiceRef;
        // FIXME add another service ref channel for deleting.
        gv_browse_callback _browseCallback;
        std::future<GV_ERROR> _futHandleEvents;
        std::vector<DNSServiceRef> _vecOpenBrowseRefs;
        std::map<std::string, DNSServiceRef> _mapOpenResolveRefs;
        std::map<std::string, DNSServiceRef> _mapOpenRegisterRefs;

        static GV_ERROR handleEvents(
            IN UPCHServiceRef const *pupchAddServiceRef,
            IN UPCHServiceRef const *pupchRemoveServiceRef);
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

using UPZeroconfClient = std::unique_ptr<ZeroconfClient>;
using CHServiceRef = Channel<DNSServiceRef, UPServiceRefDeleter>;

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_ZEROCONF_H_

