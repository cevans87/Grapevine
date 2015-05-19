#include "gv_communicator.h"

namespace grapevine {

Communicator::Communicator(
) {
}

GV_ERROR
Communicator::make_publisher(
    IN char const *pszPublisherName
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;

    // TODO error checking on pszPublisherName before handing it off to
    // zmq_client. Notably, no underscores. Look at naming conventions.
    error = _zmqClient.make_publisher(_zeroconfClient, pszPublisherName);

    GV_PRINT(EXIT, "");
    return error;
}

GV_ERROR
Communicator::make_subscriber(
    IN char const *pszSubscriberName
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;

    // TODO error checking on pszSubscriberName before handing it off to
    // zmq_client.
    error = _zmqClient.make_subscriber(_zeroconfClient, pszSubscriberName);

    GV_PRINT(EXIT, "");
    return error;
}

GV_ERROR
Communicator::get_next_message(
    IN char const *pszPublisherName,
    OUT zmq::message_t *pMsg
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;

    // TODO error check pszPublisherName.
    // TODO Add flags for nowait vs block.
    error = _zmqClient.get_next_message(pszPublisherName, pMsg);

    GV_PRINT(EXIT, "");
    return error;
}

//GV_ERROR
//Communicator::get_last_message(
//    IN char const &service,
//    OUT zmq::message_t *pMsg
//) noexcept {
//    return GV_ERROR::SUCCESS;
//}

GV_ERROR
Communicator::publish_message(
    IN char const *pszPublisherName,
    IN char *pszMsg
) noexcept {
    GV_PRINT(ENTRY, "");
    GV_ERROR error = GV_ERROR::SUCCESS;

    // TODO error check pszPublisherName and pMsg
    error = _zmqClient.publish_message(pszPublisherName,
            pszMsg, strlen(pszMsg) + 1);

    GV_PRINT(EXIT, "");
    return error;
}

} // namespace grapevine
