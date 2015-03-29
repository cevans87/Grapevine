#include <time.h>
#include <sys/select.h>
#include <assert.h>

#include <future>
#include <functional>

#include "gv_zeroconf.hpp"
#include "gv_util.hpp"

namespace grapevine {

void
ZeroconfClient::browseCallback(
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
    IN DNSServiceRef service,
    IN DNSServiceFlags flags,
    IN uint32_t interfaceIndex,
    IN DNSServiceErrorType errorCode,
    IN const char *name,
    IN const char *type,
    IN const char *domain,
    IN void *context
#pragma clang diagnostic pop
    )
{
    ZeroconfClient *self = reinterpret_cast<ZeroconfClient *>(context);
    GV_DEBUG_PRINT("Browse callback initiated");
    if (nullptr == self)
    {
        GV_DEBUG_PRINT("No context given.");
        return;
    }
    //else if (nullptr == self->_callback)
    //{
    //    GV_DEBUG_PRINT("No callback set.");
    //    return;
    //}
}

GV_ERROR
ZeroconfClient::setBrowseCallback(
    IN gv_browse_callback callback
    )
{
    _browseCallback = callback;
    return GV_ERROR_SUCCESS;
}

GV_ERROR
ZeroconfClient::enableBrowse()
{
    DNSServiceErrorType error;
    GV_DEBUG_PRINT("About to call zeroconf browse");
    error = DNSServiceBrowse(
            &_serviceRef,   // sdRef,
            0,              // flags,
            0,              // interfaceIndex,
            "_grapevine._tcp", // regtype,
            // FIXME cevans87: Implement more than link-local
            "local",       // domain,
            _browseCallback,
            reinterpret_cast<void *>(this)            // context
            );
    GV_DEBUG_PRINT("DNSServiceBrowse returned with error: %d", error);
    if (!error)
    {
        printf("about to call async\n");
        _futureHandleEvents =
                std::async(std::launch::async, handleEvents, _serviceRef);
        printf("Called async\n");
        // FIXME _serviceRef isn't deallocated anywere.
    }
    return GV_ERROR_SUCCESS;
}

void
ZeroconfClient::handleEvents(
    IN DNSServiceRef serviceRef
    )
{
    int err = 0;
    struct timeval tv;
    fd_set readfds;
    int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
    int nfds = dns_sd_fd + 1; // FIXME, this info should come from somewhere else.
    while (!err)
    {
        FD_ZERO(&readfds);
        FD_SET(dns_sd_fd, &readfds);
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        // FIXME add a kill FD to signal that owner object is dying?
        int result = select(
                nfds, // TODO mark these
                &readfds,
                nullptr,
                nullptr,
                &tv
                );
        if (result > 0 && FD_ISSET(dns_sd_fd, &readfds))
        {
            // FIXME future destructor at end of loop is blocking. Create
            // thread without blocking.
            printf("found something interesting\n");
            std::future<int> handle = std::async(std::launch::async,
                    DNSServiceProcessResult, serviceRef);
            //err = DNSServiceProcessResult(serviceRef);
        }
    }
}

} // namespace grapevine
