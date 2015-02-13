#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include "gv_browser.hpp"
#include "gv_util.hpp"

GV_Browser::GV_Browser()
{
    DNSServiceErrorType error;
    GV_DEBUG_PRINT("about to browse");
    error = browse();
// FIXME cevans87: This doesn't have the desired effect... Shut up Avahi!
#ifdef GV_WITH_AVAHI
    setenv("AVAHI_COMPAT_NOWARN", "1", true);
#endif // GV_WITH_AVAHI
    if (error)
    {
        GV_DEBUG_PRINT("DNSServiceErrorType error (%d) on browse", error);
    }
}

void
GV_Browser::callback(
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
    const char *addString = (flags & kDNSServiceFlagsAdd) ? "ADD" : "REMOVE";
    const char *moreString = (flags & kDNSServiceFlagsMoreComing) ? "MORE" : "    ";
    if (errorCode != kDNSServiceErr_NoError)
    {
        GV_DEBUG_PRINT("DNSServiceErrorType error (%d) on browse", errorCode);
        return;
    }
    printf("%-7s%-5s iface:%d %s.%s%s\n", addString, moreString, interfaceIndex, 
            name, type, domain);
}

DNSServiceErrorType
GV_Browser::browse()
{
    DNSServiceErrorType error;
    DNSServiceRef serviceRef;
    GV_DEBUG_PRINT("About to call zeroconf browse");
    error = DNSServiceBrowse(
            &serviceRef,    // sdRef,
            0,              // flags,
            0,              // interfaceIndex,
            "_http._tcp", // regtype,
            "",       // domain,
            // FIXME cevans87: make callback non-static.
            callback,       // callback,
            NULL            // context
            );
    GV_DEBUG_PRINT("Got reply from zeroconf browse");
    if (!error)
    {
        handleEvents(serviceRef);
        DNSServiceRefDeallocate(serviceRef);
        serviceRef = reinterpret_cast<DNSServiceRef>(NULL);
    }
    return error;
}

void
GV_Browser::handleEvents(
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
            err = DNSServiceProcessResult(serviceRef);
        }
    }
}
