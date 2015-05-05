#include <chrono>

#include "gv_util.h"
#include "gv_zmq.h"

#include "gtest/gtest.h"

using std::this_thread::sleep_for;
using std::chrono::seconds;

namespace gv = grapevine;

// Sends items through a channel of capacity 0.
TEST(zmq_client, publisher_binds) {
    gv::ZeroconfClient zeroconfClient;
    gv::ZMQClient zmqClient;
    zmqClient.make_publisher(zeroconfClient, "hahahaha");
    // FIXME this doesn't test that it binds.
}

TEST(zmq_client, subscriber_sees_publisher) {
    gv::ZeroconfClient zeroconfClient;
    gv::ZMQClient zmqClient;
    zmqClient.make_publisher(zeroconfClient, "hahahaha");
    zmqClient.make_subscriber(zeroconfClient, "hahahaha");
    sleep_for(seconds(5));
    // FIXME this doesn't test that it binds.
}
