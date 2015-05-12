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
    GV_PRINT(DEBUG, "got endpoint: %s", buf);
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
    char serviceBuf[64];  // FIXME what is the actual limit on service name len?
    char hostBuf[64]; // FIXME what is the actual limit on service name len?
    char targetBuf[64]; // FIXME what is the actual limit on service name len?

    size_t nBytes = strlen(pszServiceName) + 1;
    // TODO check math
    nBytes = (nBytes <= 64) ? nBytes : 64;

    strncpy(serviceBuf, pszServiceName, nBytes);
    *strchr(serviceBuf, '.') = '\0';

    nBytes = strlen(pszHostName) + 1;
    // TODO check math
    nBytes = (nBytes <= 64) ? nBytes : 64;

    strncpy(hostBuf, pszHostName, nBytes);
    *strchr(hostBuf, '.') = '\0';

    GV_PRINT(DEBUG, "Found a registered service %s at %s:%u",
            pszServiceName, pszHostName, ntohs(uPort));
    GV_PRINT(DEBUG, "Buf: %s", serviceBuf);
    // FIXME pszServiceName is <service>._grapevine._tcp.local. we just want
    // the <service> part. This won't work the way it is.
    if (_mapSubscribers.end() != _mapSubscribers.find(serviceBuf)) {
        snprintf(targetBuf, sizeof(targetBuf) - 1, "tcp://%s:%d", hostBuf, ntohs(uPort));
        GV_PRINT(DEBUG, "connecting to %s", targetBuf);
        _mapSubscribers.at(serviceBuf).upSubscriber->connect(targetBuf);
        GV_PRINT(DEBUG, "Finished connect");
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

GV_ERROR
ZMQClient::publish_message(
    IN char const *pszPublisherName,
    IN void *pMsg,
    IN size_t msgLen
) {
    GV_ERROR error = GV_ERROR::SUCCESS;

    if (_mapPublishers.end() == _mapPublishers.find(pszPublisherName)) {
        error = GV_ERROR::KEY_MISSING;
        GV_BAIL(error, ERROR);
    } else {
        zmq::message_t msg(pMsg, msgLen, nullptr);
        _mapPublishers.at(pszPublisherName).upPublisher->send(msg, 0);
    }

out:
    return error;

error:
    goto out;
}

GV_ERROR
ZMQClient::get_next_message(
    IN char const *pszSubscriberName,
    OUT zmq::message_t *pMsg
) {
    GV_ERROR error = GV_ERROR::SUCCESS;

    if (_mapSubscribers.end() == _mapSubscribers.find(pszSubscriberName)) {
        error = GV_ERROR::KEY_MISSING;
        GV_BAIL(error, ERROR);
    } else {
        _mapSubscribers.at(pszSubscriberName).upSubscriber->recv(pMsg, 0);
        GV_PRINT(DEBUG, "Got message %s", pMsg->data());
    }


out:
    return error;

error:
    goto out;
}

} // namespace grapevine
