#include <time.h>
#include <sys/select.h>
#include <assert.h>
#include <unistd.h>

#include <chrono>
#include <future>
#include <functional>
#include <map>

#include "gv_zeroconf.hpp"
#include "gv_util.hpp"

using std::unique_ptr;
using std::pair;
using std::map;
using std::future;
using std::launch;

namespace grapevine {

ZeroconfClient::ZeroconfClient(
    )
{
    int eventHandlerPipeFd[2];

    _upchAddServiceRef =
            UP_Channel<DNSServiceRef>(new Channel<DNSServiceRef>(0));

    pipe(eventHandlerPipeFd);

    unique_ptr<int> upHandlerFd(new int(eventHandlerPipeFd[1]));

    _futHandleEvents = async(
            launch::async,
            handleEvents,
            //eventHandlerPipeFd[0], TODO delete
            // FIXME I'd rather use a const rvalue here, but can't use that in
            // an async call. What's the right way to do this?
            &_upchAddServiceRef);
}

ZeroconfClient::~ZeroconfClient(
    )
{
    GV_DEBUG_PRINT("Trying to die\n");

    _upchAddServiceRef->close();
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
    //GV_DEBUG_PRINT("DNSServiceBrowse returned with error: %d", serviceError);
    unique_ptr<DNSServiceRef> upServiceRef(new DNSServiceRef(serviceRef));

    error = _upchAddServiceRef->put(&upServiceRef);
    printf("Finished putting fd\n");
    BAIL_ON_GV_ERROR(error);

out:
    return error;

error:
    goto out;
}

GV_ERROR
ZeroconfClient::handleEvents(
    IN UP_Channel<DNSServiceRef> const *pupchAddServiceRef
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    int iChanFd = -1;
    int maxFd;
    fd_set readFds;
    map<int, unique_ptr<DNSServiceRef>> mapFdToServiceRef;

    // Select on readFds until pupchAddServiceRef closes
    while (true) {
        error = (*pupchAddServiceRef)->get_notify_data_available_fd(&iChanFd);
        BAIL_ON_GV_ERROR(error);

        FD_ZERO(&readFds);
        FD_SET(iChanFd, &readFds);
        maxFd = iChanFd;
        for (pair<int const, unique_ptr<DNSServiceRef>> const &fdToRef :
                mapFdToServiceRef) {

            FD_SET(fdToRef.first, &readFds);
            maxFd = (maxFd >= fdToRef.first) ? maxFd : fdToRef.first;
        }
        int result = select(
                maxFd + 1,
                &readFds, // All we wanna do is read
                nullptr,  // Don't care about writes
                nullptr,  // Don't care about exceptions
                nullptr   // Don't time out
                );
        if (result > 0) {
            if (FD_ISSET(iChanFd, &readFds)) {
                // Something waiting on the channel.
                unique_ptr<DNSServiceRef> upServiceRef;
                error = (*pupchAddServiceRef)->get(&upServiceRef);
                BAIL_ON_GV_ERROR(error);

                int dnssdFd = DNSServiceRefSockFD(*upServiceRef);
                mapFdToServiceRef.insert(
                        pair<int, unique_ptr<DNSServiceRef>>(
                            dnssdFd,
                            move(upServiceRef)));
            }
            for (pair<int const, unique_ptr<DNSServiceRef>> const &fdToRef :
                    mapFdToServiceRef) {
                if (FD_ISSET(fdToRef.first, &readFds)) {
                    // this async doesn't return, we're screwed next loop.

                    // Spawns thread to do the work.

                    // FIXME this spawns a thread. if the thread doesn't read from the socket real
                    future<DNSServiceErrorType> handle = async(
                            launch::async,
                            DNSServiceProcessResult,
                            *fdToRef.second);
                    //DNSServiceProcessResult(*fdToRef.second);

                    // FIXME we block here on std::~future. Fix this.
                }
            }
        }
    }

out:
    for (pair<int const, unique_ptr<DNSServiceRef>> const &fdToRef :
            mapFdToServiceRef) {

        // FIXME include the correct destructor in the unique pointer
        // construction so we don't have to do this cleanup.
        DNSServiceRefDeallocate(*fdToRef.second);
    }
    if (-1 != iChanFd) {
        (*pupchAddServiceRef)->close_notify_data_available_fd(&iChanFd);
    }
    printf("returning from handler Thread\n");
    return error;

error:
    goto out;
}

} // namespace grapevine
