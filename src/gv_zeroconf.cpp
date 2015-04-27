#include <time.h>
#include <sys/select.h>
#include <assert.h>
#include <unistd.h>

#include <chrono>
#include <future>
#include <functional>
#include <map>

#include "gv_zeroconf.h"
#include "gv_util.h"

using std::unique_ptr;
using std::pair;
using std::map;
using std::future;
using std::launch;
using std::make_unique;

namespace grapevine {

ZeroconfClient::ZeroconfClient(
) {
    _upchAddServiceRef = make_unique<CHServiceRef>(_ukChannelSize);

    _upchRemoveServiceRef = make_unique<CHServiceRef>(_ukChannelSize);

    _futHandleEvents = async(
            launch::async,
            handleEvents,
            // FIXME I'd rather use a const rvalue here, but can't use that in
            // an async call. What's the right way to do this?
            &_upchAddServiceRef,
            &_upchRemoveServiceRef);
}

ZeroconfClient::~ZeroconfClient(
) {
    GV_DEBUG_PRINT("Trying to die\n");

    _upchAddServiceRef->close();
    _upchRemoveServiceRef->close();
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
) {
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

// FIXME remove this function and just require it in the enableBrowse function.
GV_ERROR
ZeroconfClient::setBrowseCallback(
    IN gv_browse_callback callback
) {
    _browseCallback = callback;
    return GV_ERROR_SUCCESS;
}

GV_ERROR
ZeroconfClient::addRegisterCallback(
    IN char const *pszServiceName,
    IN gv_register_callback callback
) {
    GV_ERROR error = GV_ERROR_SUCCESS;
    DNSServiceErrorType serviceError;
    DNSServiceRef serviceRef = nullptr;
    UPServiceRef upServiceRef = nullptr;

    // FIXME WE NEED to get a port and bind to it!

    if (_mapOpenRegisterRefs.find(pszServiceName) != _mapOpenRegisterRefs.end()) {
        error = GV_ERROR_KEY_CONFLICT;
        BAIL_ON_GV_ERROR(error);
    }

    serviceError = DNSServiceRegister(
            &serviceRef,                    // sdRef,
            0,                              // flags,
            // TODO support passing kDNSServiceInterfaceIndexLocalOnly if we
            // set up our communicator for loopback instead of local network.
            // Might want to do this soon to make testing less obnoxious.
            0,                              // interfaceIndex,
            pszServiceName,                 // name,
            "_grapevine._tcp",              // regtype,
            // TODO Implement more than link-local
            "local",                        // domain,
            NULL,                           // host,
            // FIXME need to get a port!
            0,                              // port,
            0,                              // txtLen,
            NULL,                           // txtRecord,
            callback,                       // callback,
            reinterpret_cast<void *>(this)  // context
            );
    // FIXME handle serviceError
    upServiceRef = make_unique<ServiceRef>(serviceRef);
    _mapOpenRegisterRefs.emplace(pszServiceName, serviceRef);

    error = _upchAddServiceRef->put(&upServiceRef);
    printf("Finished putting fd\n");
    BAIL_ON_GV_ERROR(error);
out:
    return error;

error:
    goto out;
}


GV_ERROR
ZeroconfClient::addResolveCallback(
    IN char const *pszServiceName,
    IN gv_resolve_callback callback
) {
    GV_ERROR error = GV_ERROR_SUCCESS;
    DNSServiceErrorType serviceError;
    DNSServiceRef serviceRef = nullptr;
    UPServiceRef upServiceRef = nullptr;

    if (_mapOpenResolveRefs.find(pszServiceName) != _mapOpenResolveRefs.end()) {
        error = GV_ERROR_KEY_CONFLICT;
        BAIL_ON_GV_ERROR(error);
    }

    serviceError = DNSServiceResolve(
            &serviceRef,                    // sdRef,
            0,                              // flags,
            0,                              // interfaceIndex,
            pszServiceName,                 // name,
            "_grapevine._tcp",              // regtype,
            // TODO Implement more than link-local
            "local",                        // domain,
            callback,                       // callback,
            reinterpret_cast<void *>(this)  // context
            );
    // FIXME handle serviceError
    upServiceRef = make_unique<ServiceRef>(serviceRef);
    _mapOpenResolveRefs.emplace(pszServiceName, serviceRef);

    error = _upchAddServiceRef->put(&upServiceRef);
    printf("Finished putting fd\n");
    BAIL_ON_GV_ERROR(error);
out:
    return error;

error:
    goto out;
}

GV_ERROR
ZeroconfClient::enableBrowse(
) {
    GV_ERROR error = GV_ERROR_SUCCESS;
    DNSServiceErrorType serviceError;
    DNSServiceRef serviceRef = nullptr;

    GV_DEBUG_PRINT("About to call zeroconf browse");
    serviceError = DNSServiceBrowse(
            &serviceRef,                    // sdRef,
            0,                              // flags,
            0,                              // interfaceIndex,
            "_grapevine._tcp",              // regtype,
            // TODO Implement more than link-local
            "local",                        // domain,
            _browseCallback,                // callback,
            reinterpret_cast<void *>(this)  // context
            );
    // FIXME handle serviceError
    UPServiceRef upServiceRef = make_unique<ServiceRef>(serviceRef);
    _vecOpenBrowseRefs.push_back(serviceRef);

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
    IN UPCHServiceRef const *pupchAddServiceRef,
    IN UPCHServiceRef const *pupchRemoveServiceRef
) {
    GV_ERROR error = GV_ERROR_SUCCESS;
    int fdAddRef = -1;
    int fdRemoveRef = -1;
    int maxFd;
    fd_set readFds;
    map<int, UPServiceRef> mapFdToServiceRef;

    // Select on readFds until pupchAddServiceRef closes
    while (true) {
        error = (*pupchAddServiceRef)->get_notify_data_available_fd(&fdAddRef);
        BAIL_ON_GV_ERROR(error);
        error = (*pupchRemoveServiceRef)->get_notify_data_available_fd(&fdRemoveRef);
        BAIL_ON_GV_ERROR(error);

        FD_ZERO(&readFds);
        FD_SET(fdAddRef, &readFds);
        FD_SET(fdRemoveRef, &readFds);
        maxFd = (fdAddRef > fdRemoveRef) ? fdAddRef : fdRemoveRef;
        for (pair<int const, UPServiceRef> const &fdToRef :
                mapFdToServiceRef) {

            FD_SET(fdToRef.first, &readFds);
            maxFd = (maxFd >= fdToRef.first) ? maxFd : fdToRef.first;
        }
        int result = select(maxFd + 1, &readFds, nullptr, nullptr, nullptr);
        if (result > 0) {
            if (FD_ISSET(fdAddRef, &readFds)) {
                // Something waiting on the channel.
                UPServiceRef upServiceRef;
                error = (*pupchAddServiceRef)->get(&upServiceRef);
                BAIL_ON_GV_ERROR(error);

                int dnssdFd = DNSServiceRefSockFD(upServiceRef->ref);
                //mapFdToServiceRef.emplace(dnssdFd, move(upServiceRef));
                mapFdToServiceRef.insert(std::pair<int, UPServiceRef>(dnssdFd, move(upServiceRef)));
            }
            if (FD_ISSET(fdRemoveRef, &readFds)) {
                UPServiceRef upServiceRef;
                error = (*pupchRemoveServiceRef)->get(&upServiceRef);
                BAIL_ON_GV_ERROR(error);

                int dnssdFd = DNSServiceRefSockFD(upServiceRef->ref);
                mapFdToServiceRef.erase(dnssdFd);
            }
            for (pair<int const, UPServiceRef> const &fdToRef :
                    mapFdToServiceRef) {
                if (FD_ISSET(fdToRef.first, &readFds)) {
                    // this async doesn't return, we're screwed next loop.

                    // Spawns thread to do the work.

                    // FIXME this spawns a thread. if the thread doesn't read from the socket real
                    future<DNSServiceErrorType> handle = async(
                            launch::async,
                            DNSServiceProcessResult,
                            (fdToRef.second)->ref);
                    //DNSServiceProcessResult(*fdToRef.second);

                    // FIXME we block here on std::~future. Fix this.
                }
            }
        }
    }

out:
    if (-1 != fdAddRef) {
        (*pupchAddServiceRef)->close_notify_data_available_fd(&fdAddRef);
    }
    if (-1 != fdRemoveRef) {
        (*pupchRemoveServiceRef)->close_notify_data_available_fd(&fdRemoveRef);
    }
    printf("returning from handler Thread\n");
    return error;

error:
    goto out;
}

} // namespace grapevine
