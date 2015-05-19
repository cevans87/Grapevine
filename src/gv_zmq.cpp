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

static constexpr char const g_pszAddrFmt[] = "tcp://%s:%i";
static constexpr unsigned long const g_ulMaxAddrLen = kDNSServiceMaxDomainName +
        sizeof(g_pszAddrFmt) - 4 + 5; // subtract 4 fmt chars, add 5 portnum chars.

ZMQClient::ZMQClient(
) noexcept :
    _context(1)
{
    GV_PRINT(ENTRY, "");
    _bRegistered = false;
    GV_PRINT(EXIT, "");
}

ZMQClient::ZMQClient(
    IN int iIOThreads
) noexcept :
    _context(iIOThreads)
{
    GV_PRINT(ENTRY, "");
    _bRegistered = false;
    GV_PRINT(EXIT, "");
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
) noexcept {
    GV_PRINT(ENTRY, "");
    lock_guard<mutex> lg(_mtx);
    GV_PRINT(INFO, "In register cb");
    if (_mapPublishers.end() != _mapPublishers.find(pszServiceName)) {
        GV_PRINT(INFO, "Publisher exists");
        _mapPublishers.at(pszServiceName).bRegistered = true;
        _cv.notify_all();
    }
    GV_PRINT(EXIT, "");
}

GV_ERROR
ZMQClient::make_publisher(
    IN ZeroconfClient &zeroconfClient,
    IN char const *pszPublisherName
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;
    char bufEndpoint[1024];
    size_t bufEndpointlen = sizeof(bufEndpoint);
    char *pszPortNum;
    unique_ptr<socket_t> upPublisher = make_unique<socket_t>(_context, ZMQ_PUB);

    upPublisher->bind("tcp://*:*");
    upPublisher->getsockopt(
            ZMQ_LAST_ENDPOINT, static_cast<void *>(bufEndpoint), &bufEndpointlen);
    // XXX if bind fails, does getsockopt also fail? It'd make the strrchr fail
    // too.
    GV_PRINT(INFO, "got endpoint: %s", bufEndpoint);
    pszPortNum = strrchr(bufEndpoint, ':') + 1;

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
        GV_PRINT(ENTRY, "");
        static_cast<ZMQClient *>(context)->register_callback(
                serviceRef,
                flags,
                errorCode,
                pszServiceName,
                pszRegType,
                pszDomainName);
        GV_PRINT(EXIT, "");
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
    GV_PRINT(EXIT, "");
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
) noexcept {
    GV_PRINT(ENTRY, "");
    lock_guard<mutex> lg(_mtx);
    char serviceBuf[kDNSServiceMaxServiceName]; // Includes space for '\0'
    char addressBuf[g_ulMaxAddrLen];

    snprintf(serviceBuf, sizeof(serviceBuf), "%s", pszServiceName);
    *strchrnul(serviceBuf, '.') = '\0';

    GV_PRINT(INFO, "Found a registered service %s at %s:%u",
            pszServiceName, pszHostName, ntohs(uPort));
    GV_PRINT(INFO, "Buf: %s", serviceBuf);
    if (_mapSubscribers.end() != _mapSubscribers.find(serviceBuf)) {
        snprintf(addressBuf, sizeof(addressBuf), g_pszAddrFmt, pszHostName, ntohs(uPort));
        GV_PRINT(INFO, "connecting to %s", addressBuf);
        _mapSubscribers.at(serviceBuf).upSubscriber->connect(addressBuf);
        _mapSubscribers.at(serviceBuf).upSubscriber->setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);
        _mapSubscribers.at(serviceBuf).bSubscribed = true;
        _cv.notify_all();
    }
    GV_PRINT(EXIT, "");
}

GV_ERROR
ZMQClient::make_subscriber(
    IN ZeroconfClient &zeroconfClient,
    IN char const *pszSubscriberName
) noexcept {
    GV_PRINT(ENTRY, "");
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
        GV_PRINT(ENTRY, "");
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
        GV_PRINT(EXIT, "");
    };

    lock_guard<mutex> lg(_mtx);
    GV_PRINT(INFO, "Adding subscriber's resolve callback for '%s'",
            pszSubscriberName);
    zeroconfClient.add_resolve_callback(
            pszSubscriberName, // pszServiceName,
            resolve_cb, // callback
            reinterpret_cast<void *>(this)); // context

    _mapSubscribers.emplace(pszSubscriberName, &upSubscriber);

//out:
    GV_PRINT(EXIT, "");
    return error;

//error:
//    goto out;
}

GV_ERROR
ZMQClient::publish_message(
    IN char const *pszPublisherName,
    IN void *pMsg,
    IN size_t msgLen
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;

    if (_mapPublishers.end() == _mapPublishers.find(pszPublisherName)) {
        error = GV_ERROR::KEY_MISSING;
        GV_BAIL(error, ERROR);
    } else {
        zmq::message_t msg(pMsg, msgLen, nullptr);
        // FIXME handle send errors.
        _mapPublishers.at(pszPublisherName).upPublisher->send(msg, 0);
    }

out:
    GV_PRINT(EXIT, "");
    return error;

error:
    goto out;
}

GV_ERROR
ZMQClient::get_next_message(
    IN char const *pszSubscriberName,
    OUT zmq::message_t *pMsg
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;
    unique_lock<mutex> ul(_mtx);
    bool done = false;

    do {
        if (_mapSubscribers.end() == _mapSubscribers.find(pszSubscriberName)) {
            error = GV_ERROR::KEY_MISSING;
            GV_BAIL(error, ERROR);
        } else if (false == _mapSubscribers.at(pszSubscriberName).bSubscribed) {
            GV_PRINT(INFO, "Subscribe to '%s' not completed, waiting",
                    pszSubscriberName);
            _cv.wait(ul);
        } else {
            // FIXME, since we want to be able to reconnect elsewhere, we can't
            // afford to let this call block. Find a good way to interrupt the recv?
            done = _mapSubscribers.at(pszSubscriberName).upSubscriber->recv(pMsg, 0);
            GV_PRINT(INFO, "Got message %s", pMsg->data());
        }
    } while (false == done);


out:
    GV_PRINT(EXIT, "");
    return error;

error:
    goto out;
}

} // namespace grapevine
