#ifndef GRAPEVINE_SRC_GV_ZMQ_H_
#define GRAPEVINE_SRC_GV_ZMQ_H_

#include <map>
#include <zmq.hpp>

#include "gv_zeroconf.h"
#include "gv_util.h"

namespace grapevine {

struct Publisher
{
    std::unique_ptr<zmq::socket_t> upPublisher;
    bool bRegistered;

    Publisher() = delete;
    Publisher(
        IN std::unique_ptr<zmq::socket_t> *pupPublisher
    ) {
        upPublisher = move(*pupPublisher);
        bRegistered = false;
    }
};

// XXX this is only holding a pointer at the moment. I think we'll put more
// data in here at some point.
struct Subscriber
{
    std::unique_ptr<zmq::socket_t> upSubscriber;
    Subscriber() = delete;
    Subscriber(
        IN std::unique_ptr<zmq::socket_t> *pupSubscriber
    ) {
        upSubscriber = move(*pupSubscriber);
    }
};

class ZMQClient
{
    public:
        ZMQClient();
        explicit ZMQClient(
            IN int iIOThreads);

        GV_ERROR make_publisher(
            IN ZeroconfClient &zeroconfClient,
            IN char const *pszPublisherName);

        GV_ERROR make_subscriber(
            IN ZeroconfClient &zeroconfClient,
            IN char const *pszSubscriberName);

        void register_callback(
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszRegType,
            IN char const *pszDomainName);

        void resolve_callback(
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN uint32_t uInterfaceIndex,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszHostName,
            IN uint16_t uPort,
            IN uint16_t uTxtLen,
            IN unsigned char const *pszTxtRecord);

        GV_ERROR publish_message(
            IN char const *pszPublisherName,
            IN void *pMsg,
            IN size_t msgLen);

        GV_ERROR get_next_message(
            IN char const *pszSubscriberName,
            OUT zmq::message_t *msg);

        //GV_ERROR make_subscriber(
        //    IN char const *pszName);
        //GV_ERROR get_last_broadcast(
        //    IN char const *pszName,
        //    OUT zmq_msg_t *pMsg);

    private:
        std::mutex _mtx;
        std::condition_variable _cv;
        bool _bRegistered;


        // XXX Any point in changing number of IO threads for context?
        zmq::context_t _context;
        std::map<std::string, Publisher> _mapPublishers;
        std::map<std::string, Subscriber> _mapSubscribers;

        GV_ERROR get_bind_string(
            OUT std::unique_ptr<char> *pupszBindString);
};

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_ZMQ_H_

