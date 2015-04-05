#ifndef GRAPEVINE_SRC_GV_ZEROCONF_HPP_
#define GRAPEVINE_SRC_GV_ZEROCONF_HPP_

#include <dns_sd.h>
#include <functional>
#include <future>

#include "gv_type.hpp"
#include "gv_channel.hpp"

namespace grapevine {

using gv_browse_callback = DNSServiceBrowseReply;
using gv_resolve_callback = DNSServiceResolveReply;
using gv_register_callback = DNSServiceRegisterReply;

using gv_browse_context = void *;

class ZeroconfClient
{
    public:
        ZeroconfClient();
        ~ZeroconfClient();
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
        // Mutex needed to keep mul

        UP_Channel<DNSServiceRef> _upchAddServiceRef;
        gv_browse_callback _browseCallback;
        std::future<GV_ERROR> _futEventHandler;

        static GV_ERROR eventHandlerThread(
            IN UP_Channel<DNSServiceRef> const *pupchAddServiceRef);
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

using UP_ZeroconfClient = std::unique_ptr<ZeroconfClient>;

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_ZEROCONF_HPP_

