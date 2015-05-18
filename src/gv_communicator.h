#ifndef GRAPEVINE_SRC_GV_COMMUNICATOR_H_
#define GRAPEVINE_SRC_GV_COMMUNICATOR_H_

#include "gv_util.h"
#include "gv_zmq.h"
#include "gv_zeroconf.h"

namespace grapevine {

class Communicator
{
    public:
        Communicator();
        GV_ERROR make_publisher(
                IN char const *pszPublisherName) noexcept;

        GV_ERROR make_subscriber(
                IN char const *pszSubscriberName) noexcept;

        //GV_ERROR get_services(
        //        OUT UP_GV_Services *upServices);

        GV_ERROR get_next_message(
                IN char const *pszPublisherName,
                OUT zmq::message_t *pMsg) noexcept;
        //GV_ERROR get_last_message(
        //        IN char const *pszPublisherName,
        //        OUT zmq::message_t *pMsg) noexcept;

        GV_ERROR publish_message(
                IN char const *pszPublisherName,
                IN char *pszMsg) noexcept;
    private:
        ZMQClient _zmqClient;
        ZeroconfClient _zeroconfClient;

};

using UPCommunicator = std::unique_ptr<Communicator>;

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_COMMUNICATOR_H_

