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
using zmq::socket_t;

ZMQClient::ZMQClient(
) :
    _context(1)
{
    _bRegistered = false;
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
    if (_mapPublishers.end() != _mapPublishers.find(pszServiceName)) {
        _mapPublishers.at(pszServiceName).bRegistered = true;
        _cv.notify_all();
    }
}

GV_ERROR
ZMQClient::make_publisher(
    IN ZeroconfClient &zeroconfClient,
    IN char const *pszPublisherName
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    char buf[1024];
    size_t buflen = sizeof(buf);
    char *pszPortNum;
    unique_ptr<socket_t> upPublisher = make_unique<socket_t>(_context, ZMQ_PUB);

    upPublisher->bind("tcp://*:*");
    upPublisher->getsockopt(ZMQ_LAST_ENDPOINT, static_cast<void *>(buf), &buflen);
    GV_DEBUG_PRINT("got endpoint: %s", buf);
    pszPortNum = strrchr(buf, ':') + 1;

    gv_register_callback register_cb = [](
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
        static_cast<ZMQClient *>(context)->register_callback(
                serviceRef,
                flags,
                errorCode,
                pszServiceName,
                pszRegType,
                pszDomainName);
    };

    lock_guard<mutex> lg(_mtx);

    zeroconfClient.add_register_callback(
            0, // flags
            0, // uInterfaceIndex,
            nullptr, // pszDomainName,
            nullptr, // pszHostName,
            pszPublisherName, // pszServiceName,
            htons(static_cast<unsigned short>(atoi(pszPortNum))), // uPortNum,
            nullptr, // pTxtRecord
            0, // uTxtLen,
            register_cb, // callback
            reinterpret_cast<void *>(this)); // context

    _mapPublishers.emplace(pszPublisherName, &upPublisher);

    //using std::this_thread::sleep_for;
    //using std::chrono::seconds;
    //sleep_for(seconds(10));

//out:
    return error;

//error:
//    goto out;
}

void
ZMQClient::resolve_callback(
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
    IN DNSServiceRef serviceRef,
    IN DNSServiceFlags flags,
    IN uint32_t uInterfaceIndex,
    IN DNSServiceErrorType errorCode,
    IN char const *pszServiceName,
    IN char const *pszHostName,
    IN uint16_t uPort,
    IN uint16_t uTxtLen,
    IN unsigned char const *pszTxtRecord
#pragma clang diagnostic pop
) {
    lock_guard<mutex> lg(_mtx);
    GV_DEBUG_PRINT("Found a registered service %s at %s:%u",
            pszServiceName, pszHostName, ntohs(uPort));
    // FIXME pszServiceName is <service>._grapevine._tcp.local. we just want
    // the <service> part. This won't work the way it is.
    if (_mapSubscribers.end() != _mapSubscribers.find(pszServiceName)) {
        GV_DEBUG_PRINT("Yay!");
        // FIXME Maybe set a variable saying the broadcaster for this
        // subscription exists?
        _cv.notify_all();
    }
}

GV_ERROR
ZMQClient::make_subscriber(
    IN ZeroconfClient &zeroconfClient,
    IN char const *pszSubscriberName
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    unique_ptr<socket_t> upSubscriber = make_unique<socket_t>(_context, ZMQ_SUB);

    gv_resolve_callback resolve_cb = [](
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN uint32_t uInterfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszHostName,
            IN uint16_t uPort,
            IN uint16_t uTxtLen,
            IN unsigned char const *pszTxtRecord,
            IN void *context) -> void
#pragma clang diagnostic pop
    {
        static_cast<ZMQClient *>(context)->resolve_callback(
                serviceRef,
                flags,
                uInterfaceIndex,
                errorCode,
                pszServiceName,
                pszHostName,
                uPort,
                uTxtLen,
                pszTxtRecord);
    };

    lock_guard<mutex> lg(_mtx);
    zeroconfClient.add_resolve_callback(
            pszSubscriberName, // pszServiceName,
            resolve_cb, // callback
            reinterpret_cast<void *>(this)); // context

    _mapSubscribers.emplace(pszSubscriberName, &upSubscriber);

    //using std::this_thread::sleep_for;
    //using std::chrono::seconds;
    //sleep_for(seconds(10));

//out:
    return error;

//error:
//    goto out;
}

} // namespace grapevine
