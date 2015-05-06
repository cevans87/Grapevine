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
    zmqClient.make_publisher(zeroconfClient, "publisher0");
    // FIXME this doesn't test that it binds.
}

TEST(zmq_client, subscriber_sees_publisher) {
    gv::ZeroconfClient zeroconfClient;
    gv::ZMQClient zmqClient;
    char pMsg[] = "here's a message!";
    zmq::message_t msg;
    zmqClient.make_publisher(zeroconfClient, "publisher1");
    zmqClient.make_subscriber(zeroconfClient, "publisher1");

    std::function<void(void)> delay_publish = [&]() {
        while (true) {
            sleep_for(seconds(2));
            zmqClient.publish_message("publisher1", pMsg, strlen(pMsg) + 1);
            printf("sent message\n");
        }
    };
    std::future<void> fut = std::async(std::launch::async, delay_publish);

    printf("waiting for a message\n");
    zmqClient.get_next_message("publisher1", &msg);

    // FIXME this doesn't test that it binds.
}
