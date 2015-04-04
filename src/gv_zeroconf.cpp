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

    _upchHandlerSignalFd = UP_Channel<int>(new Channel<int>(1));
    _upchAddServiceRef =
            UP_Channel<DNSServiceRef>(new Channel<DNSServiceRef>(0));

    pipe(eventHandlerPipeFd);

    std::unique_ptr<int> upHandlerFd =
            std::unique_ptr<int>(new int(eventHandlerPipeFd[1]));

    _upchHandlerSignalFd->put(&upHandlerFd);

    _futEventHandler = std::async(
            std::launch::async,
            eventHandlerThread,
            eventHandlerPipeFd[0],
            // FIXME I'd rather use a const rvalue here, but can't use that in
            // an async call. What's the right way to do this?
            &_upchAddServiceRef);
}

ZeroconfClient::~ZeroconfClient(
    )
{
    std::unique_ptr<int> upHandlerSignalFd;
    int handlerMsg = -1; // Death message

    GV_DEBUG_PRINT("Trying to die\n");

    _upchHandlerSignalFd->get(&upHandlerSignalFd);
    GV_DEBUG_PRINT("1\n");

    //std::lock_guard<std::mutex> lg(_handlerSignalMtx);

    // Dear handler, Please die. sincerely, cevans87
    write(*upHandlerSignalFd, &handlerMsg, sizeof(handlerMsg));
    GV_DEBUG_PRINT("2\n");
    close(*upHandlerSignalFd);
    GV_DEBUG_PRINT("3\n");
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
    DNSServiceRef serviceRef = nullptr;
    std::unique_ptr<int> upHandlerSignalFd;
    int handlerMsg = 0; // ServiceRefInChannel
    std::unique_ptr<DNSServiceRef> upServiceRef = nullptr;
    //std::unique_ptr<int> upDnssdFd;
    //upDnssdFd =
            //std::unique_ptr<int>(new int(DNSServiceRefSockFD(serviceRef)));


    GV_DEBUG_PRINT("About to call zeroconf browse");
    serviceError = DNSServiceBrowse(
            &serviceRef,                    // sdRef,
            0,                              // flags,
            0,                              // interfaceIndex,
            "_grapevine._tcp",              // regtype,
            // TODO cevans87: Implement more than link-local
            "local",                        // domain,
            _browseCallback,                // callback,
            reinterpret_cast<void *>(this)  // context
            );
    GV_DEBUG_PRINT("DNSServiceBrowse returned with error: %d", serviceError);
    upServiceRef =
            std::unique_ptr<DNSServiceRef>(new DNSServiceRef(serviceRef));

    //std::lock_guard<std::mutex> lg(_handlerSignalMtx);

    error = _upchHandlerSignalFd->get(&upHandlerSignalFd);
    BAIL_ON_GV_ERROR(error);

    write(*upHandlerSignalFd, &handlerMsg, sizeof(handlerMsg));

    //printf("Putting dnssd fd, ref is %lu\n", static_cast<long int>(*upServiceRef));
    error = _upchAddServiceRef->put(&upServiceRef);
    printf("Finished putting fd\n");
    BAIL_ON_GV_ERROR(error);

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
out:
    _upchHandlerSignalFd->put(&upHandlerSignalFd);
    return error;

error:
    goto out;
}

GV_ERROR
ZeroconfClient::eventHandlerThread(
    IN int iChanFd,
    IN UP_Channel<DNSServiceRef> const *pupchAddServiceRef
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    int selectError = 0;
    int buf;
    //bool browseEnabled = false;
    int maxFd;
    struct timeval tv;
    fd_set readFds;
    std::vector<std::unique_ptr<DNSServiceRef>> vecServiceRefs;
    std::vector<int> vecDnssdFds;
    //int dnssdFd = DNSServiceRefSockFD(serviceRef);

    tv.tv_sec = 20;
    tv.tv_usec = 0;

    while (!selectError) {
        FD_ZERO(&readFds);
        FD_SET(iChanFd, &readFds);
        maxFd = iChanFd;
        for (int dnssdFd: vecDnssdFds) {
            FD_SET(dnssdFd, &readFds);
            maxFd = (maxFd >= dnssdFd) ? maxFd : dnssdFd;
        }
        int result = select(
                maxFd + 1, // TODO mark these
                &readFds,
                nullptr,
                nullptr,
                nullptr
                //&tv
                );
        if (result > 0) {
            if (FD_ISSET(iChanFd, &readFds)) {
                // Something waiting on the channel.
                printf("data waiting on handler fd\n");
                int numBytes;
                numBytes = read(iChanFd, &buf, sizeof(buf));
                printf("Read %i bytes, buf is %i\n", numBytes, buf);
                if (-1 == buf) {
                    GV_DEBUG_PRINT("We should die now\n");
                    goto out;
                }
                std::unique_ptr<DNSServiceRef> upServiceRef;
                error = (*pupchAddServiceRef)->get(&upServiceRef);
                BAIL_ON_GV_ERROR(error);

                int dnssdFd = DNSServiceRefSockFD(*upServiceRef);
                //printf("Got ref %lu\n", static_cast<long int>(*upServiceRef));
                vecDnssdFds.emplace_back(dnssdFd);
                printf("Got fd %i\n", vecDnssdFds.back());
                vecServiceRefs.emplace_back(move(upServiceRef));
            }
            unsigned long ixFd = 0;
            for (int dnssdFd: vecDnssdFds) {
                if (FD_ISSET(dnssdFd, &readFds)) {
                    // FIXME this is the wrong way to store this handle. If
                    // this async doesn't return, we're screwed next loop.
                    std::future<int> handle = std::async(std::launch::async,
                            DNSServiceProcessResult, *vecServiceRefs.at(ixFd));
                }
                ++ixFd;
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
    for (std::unique_ptr<DNSServiceRef> &upServiceRef: vecServiceRefs) {
        DNSServiceRefDeallocate(*upServiceRef);
    }
    close(iChanFd);
    printf("returning from handler Thread\n");
    return error;

error:
    goto out;
}

} // namespace grapevine
