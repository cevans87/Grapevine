#include <chrono> // FIXME remove

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "gv_zmq.h"

namespace grapevine {

using std::unique_ptr;
using std::make_unique;
using std::mutex;
using std::unique_lock;
using std::lock_guard;

ZMQClient::ZMQClient(
) {
    ZMQClient(1);
}

ZMQClient::ZMQClient(
    IN int iIOThreads
) :
    _context(iIOThreads)
{
    _bRegistered = false;
}

void
ZMQClient::register_callback(
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszRegType,
            IN char const *pszDomainName
#pragma clang diagnostic pop
) {
    lock_guard<mutex> lg(_mtx);
    _bRegistered = true;
    _cv.notify_all();
}


GV_ERROR
ZMQClient::make_publisher(
    IN ZeroconfClient &zeroconfClient,
    IN char const *pszName
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    size_t buflen = 1024;
    char buf[buflen];
    char *pszPortNum;
    // FIXME make vector or dict of publishers
    zmq::socket_t _publisher{_context, ZMQ_PUB};
    unique_lock<mutex> ul(_mtx);
    //unique_ptr<char> upszBindString = nullptr;

    //get_bind_string(&upszBindString);
    //_publisher.bind(upszBindString.get());
    _publisher.bind("tcp://*:*");
    _publisher.getsockopt(ZMQ_LAST_ENDPOINT, static_cast<void *>(buf), &buflen);
    GV_DEBUG_PRINT("got endpoint: %s", buf);
    pszPortNum = strrchr(buf, ':') + 1;

    DNSServiceRegisterReply register_cb = [](
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszRegType,
            IN char const *pszDomainName,
            IN void *context) -> void
#pragma clang diagnostic pop
    {
        // FIXME if the ZMQClient is destroyed first and the ZeroconfClient
        // suddenly tries to use the callback, we seg fault. This is actually a
        // common case.  Make it so this can detect that the context is already
        // destroyed.
        static_cast<ZMQClient *>(context)->register_callback(
            serviceRef,
            flags,
            errorCode,
            pszServiceName,
            pszRegType,
            pszDomainName);
    };

    zeroconfClient.add_register_callback(
            0, // flags
            0, // uInterfaceIndex,
            nullptr, // pszDomainName,
            nullptr, // pszHostName,
            pszName, // pszServiceName,
            htons(static_cast<unsigned int>(atoi(pszPortNum))), // uPortNum,
            nullptr, // pTxtRecord
            0, // uTxtLen,
            register_cb); // callback

    while (!_bRegistered) {
        _cv.wait(ul);
    }

    using std::this_thread::sleep_for;
    using std::chrono::seconds;
    sleep_for(seconds(10));

out:
    return error;

error:
    goto out;
}

} // namespace grapevine
