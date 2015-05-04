#ifndef GRAPEVINE_SRC_GV_ZMQ_H_
#define GRAPEVINE_SRC_GV_ZMQ_H_

#include <map>
#include <zmq.hpp>

#include "gv_zeroconf.h"
#include "gv_util.h"

namespace grapevine {

class ZMQClient
{
    public:
        ZMQClient();
        explicit ZMQClient(
            IN int iIOThreads);

        GV_ERROR make_publisher(
            IN ZeroconfClient &zeroconfClient,
            IN char const *pszName);

        void register_callback(
            IN DNSServiceRef serviceRef,
            IN DNSServiceFlags flags,
            IN DNSServiceErrorType errorCode,
            IN char const *pszServiceName,
            IN char const *pszRegType,
            IN char const *pszDomainName);
        //GV_ERROR publish(
        //    IN char const *pszName,
        //    IN zmq_msg_t *pMsg);

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
        //zmq::socket_t _publisher;

        GV_ERROR get_bind_string(
            OUT std::unique_ptr<char> *pupszBindString);
};

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_ZMQ_H_

