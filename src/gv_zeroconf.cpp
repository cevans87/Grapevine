#include <time.h>
#include <sys/select.h>
#include <assert.h>
#include <unistd.h>

#include <future>
#include <functional>

#include "gv_zeroconf.hpp"
#include "gv_util.hpp"

namespace grapevine {

ZeroconfClient::ZeroconfClient(
    )
{
    int eventHandlerPipeFd[2];

    _upchHandlerFd = UP_Channel<int>(new Channel<int>(1));

    pipe(eventHandlerPipeFd);

    std::unique_ptr<int> upHandlerFd = std::unique_ptr<int>(new int(eventHandlerPipeFd[1]));

    _upchHandlerFd->put_nowait(&upHandlerFd);

    _futureHandleEvents = std::async(
            std::launch::async,
            eventHandlerThread,
            eventHandlerPipeFd[0],
            &_upchHandlerFd);
}

ZeroconfClient::~ZeroconfClient(
    )
{
    std::unique_ptr<int> upHandlerFd;
    int handlerMsg = -1; // Death message

    _upchHandlerFd->get(&upHandlerFd);

    std::lock_guard<std::mutex> lg(_handlerMtx);

    // Dear handler, Please die. sincerely, cevans87
    write(*upHandlerFd, &handlerMsg, sizeof(handlerMsg));
    close(*upHandlerFd);
}

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
    if (nullptr == self) {
        GV_DEBUG_PRINT("No context given.");
        return;
    }
    //else if (nullptr == self->_callback) {
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
    GV_ERROR error = GV_ERROR_SUCCESS;
    DNSServiceErrorType serviceError;
    GV_DEBUG_PRINT("About to call zeroconf browse");
    serviceError = DNSServiceBrowse(
            &_serviceRef,   // sdRef,
            0,              // flags,
            0,              // interfaceIndex,
            "_grapevine._tcp", // regtype,
            // FIXME cevans87: Implement more than link-local
            "local",       // domain,
            _browseCallback,
            reinterpret_cast<void *>(this)            // context
            );
    GV_DEBUG_PRINT("DNSServiceBrowse returned with error: %d", serviceError);

    std::lock_guard<std::mutex> lg(_handlerMtx);

    // TODO send _serviceRef to handler thread.
    
    // FIXME we want to sent this over a channel and signal that data is ready.
    //if (!serviceError) {
    //    printf("about to call async\n");
    //    int pipeFd[2];
    //    if (0 != pipe(pipeFd)) {
    //        error = GV_ERROR_EMFILE;
    //        BAIL_ON_GV_ERROR(error);
    //    }

    //    std::unique_ptr<int> upHandlerFd = std::unique_ptr<int>(new int(pipeFd[1]));

    //    error = _upchHandlerFd->put_nowait(&upHandlerFd);
    //    BAIL_ON_GV_ERROR(error);

    //    _futureHandleEvents =
    //            std::async(std::launch::async, handleEvents, _serviceRef, pipeFd[0]);
    //    printf("Called async\n");
    //    // FIXME _serviceRef isn't deallocated anywere.
    //}
//out:
//    return error;
//
//error:
//    goto out;
}

GV_ERROR
ZeroconfClient::eventHandlerThread(
    IN int iChanFd,
    IN UP_Channel<int> const *pupchHandlerFd
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    int selectError = 0;
    bool browseEnabled = false;
    int maxFd = iChanFd;
    struct timeval tv;
    fd_set readFds;
    //int dnssdFd = DNSServiceRefSockFD(serviceRef);

    FD_ZERO(&readFds);
    //FD_SET(dnssdFd, &readFds);
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    while (!selectError) {
        int result = select(
                maxFd + 1, // TODO mark these
                &readFds,
                nullptr,
                nullptr,
                &tv
                );
        if (result > 0) {
            if (FD_ISSET(iChanFd, &readFds)) {
                // Something waiting on the channel.
                std::unique_ptr<int> upDnssdFd;
                error = (*pupchHandlerFd)->get(&upDnssdFd);
                BAIL_ON_GV_ERROR(error);
                FD_SET(*upDnssdFd, &readFds);
                maxFd = (maxFd >= *upDnssdFd) ? maxFd : *upDnssdFd;

                // TODO 
            }
            //else if (FD_ISSET(dnssdFd, &readFds)) {
            //    // FIXME how should we store all the futures? Should we really
            //    // store a new thread for each fd?

            //    std::future<int> handle = std::async(std::launch::async,
            //            DNSServiceProcessResult, serviceRef);
            //}
        }
    }

out:
    return error;

error:
    goto out;
}

} // namespace grapevine
