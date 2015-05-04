#include "gv_util.h"
#include "gv_zmq.h"

#include "gtest/gtest.h"

namespace gv = grapevine;

// Sends items through a channel of capacity 0.
TEST(zmq_client, publisher_binds) {
    gv::ZeroconfClient zeroconfClient;
    gv::ZMQClient zmqClient;
    zmqClient.make_publisher(zeroconfClient, "hahahaha");
}
