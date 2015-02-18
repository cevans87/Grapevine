#include <time.h>
#include <sys/select.h>
#include <assert.h>

#include <future>
#include <functional>

#include "gv_zeroconf.hpp"
#include "gv_util.hpp"

void
GV_MDNSHandler::browseCallback(
    IN DNSServiceRef service,
    IN DNSServiceFlags flags,
    IN uint32_t interfaceIndex,
    IN DNSServiceErrorType errorCode,
    IN const char *name,
    IN const char *type,
    IN const char *domain,
    IN void *context
    )
{
    GV_MDNSHandler *self = reinterpret_cast<GV_MDNSHandler *>(context);
    GV_DEBUG_PRINT("Browse callback initiated");
    if (nullptr == self)
    {
        GV_DEBUG_PRINT("No context given.");
        return;
    }
    //else if (nullptr == self->_mCallback)
    //{
    //    GV_DEBUG_PRINT("No callback set.");
    //    return;
    //}
}

GV_ERROR
GV_MDNSHandler::setBrowseCallback(
    IN gv_browse_callback callback
    )
{
    _browseCallback = callback;
    return GV_ERROR_SUCCESS;
}

GV_ERROR
GV_MDNSHandler::enableBrowse()
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
        handleEvents(_serviceRef);
        DNSServiceRefDeallocate(_serviceRef);
        _serviceRef = reinterpret_cast<DNSServiceRef>(NULL);
    }
    return GV_ERROR_SUCCESS;
}

void
GV_MDNSHandler::handleEvents(
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
        GV_DEBUG_PRINT("handleEvents loop");
        int result = select(nfds, &readfds, reinterpret_cast<fd_set *>(NULL),
                reinterpret_cast<fd_set *>(NULL), &tv);
        if (result > 0 && FD_ISSET(dns_sd_fd, &readfds))
        {
            auto handle = std::async(std::launch::async,
                    DNSServiceProcessResult, serviceRef);
            //err = DNSServiceProcessResult(serviceRef);
        }
    }
}
