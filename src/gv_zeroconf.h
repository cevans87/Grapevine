#ifndef GRAPEVINE_SRC_GV_ZEROCONF_H_
#define GRAPEVINE_SRC_GV_ZEROCONF_H_

#include <functional>
#include <future>
#include <string>

#include <dns_sd.h>

#include "gv_util.h"
#include "gv_channel.hpp"

namespace grapevine {

using gv_browse_callback = DNSServiceBrowseReply;
using gv_resolve_callback = DNSServiceResolveReply;
using gv_register_callback = DNSServiceRegisterReply;

struct ServiceRef {
    DNSServiceRef ref;
    ServiceRef() = delete;
    ServiceRef(DNSServiceRef const serviceRef) {
        ref = serviceRef;
    }
    ~ServiceRef() {
        DNSServiceRefDeallocate(ref);
    }
};

using UPServiceRef = std::unique_ptr<ServiceRef>;
using UPCHServiceRef = UPChannel<ServiceRef>;

class ZeroconfClient
{
    public:
        ZeroconfClient();
        ~ZeroconfClient();

        // Be aware that the browse callback may be called again very quickly,
        // possibly while the previous call is still running. The callback
        // should be prepared for this.
        GV_ERROR set_browse_callback(
            IN DNSServiceBrowseReply callback);
        GV_ERROR enable_browse();
        GV_ERROR disable_browse();

        GV_ERROR add_resolve_callback(
            IN char const *pszServiceName,
            IN gv_resolve_callback callback);
        GV_ERROR enable_resolve(
            IN char *pszServiceName);
        GV_ERROR disable_resolve(
            IN char *pszServiceName);

        GV_ERROR add_register_callback(
            IN DNSServiceFlags flags,
            IN uint32_t uInterfaceIndex,
            IN char const *pszDomainName,
            IN char const *pszHostName,
            IN char const *pszServiceName,
            IN uint16_t uPortNum,
            IN unsigned char const *pTxtRecord,
            IN uint16_t uTxtLen,
            IN gv_register_callback callback);
        GV_ERROR enable_register(
            IN char *pszServiceName);
        GV_ERROR disable_register(
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

        static GV_ERROR handle_events(
            IN UPCHServiceRef const *pupchAddServiceRef,
            IN UPCHServiceRef const *pupchRemoveServiceRef);
        static void browse_callback(
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
using CHServiceRef = Channel<ServiceRef>;

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_ZEROCONF_H_

