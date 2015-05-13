#include <mutex>
#include <chrono>

#include "gv_util.h"
#include "gv_zmq.h"

#include "gtest/gtest.h"

using std::this_thread::sleep_for;
using std::chrono::seconds;
using std::mutex;
using std::unique_lock;

namespace gv = grapevine;

static mutex *g_pMtx = nullptr;

TEST(zmq_client, subscriber_gets_a_message) {
    gv::ZeroconfClient zeroconfClient;
    gv::ZMQClient zmqClient;
    char pMsg[] = "here's a message!";
    zmq::message_t msg;

    if (nullptr == g_pMtx) {
        g_pMtx = new mutex();
    }
    unique_lock<mutex> ulOuter(*g_pMtx);

    zmqClient.make_publisher(zeroconfClient, "publisher1");
    zmqClient.make_subscriber(zeroconfClient, "publisher1");

    std::function<void(void)> delay_publish = [&zmqClient, &pMsg]() {
        unique_lock<mutex> ulInner(*g_pMtx, std::defer_lock);
        while (!ulInner.try_lock()) {
            sleep_for(seconds(2));
            zmqClient.publish_message("publisher1", pMsg, strlen(pMsg) + 1);
        }
        delete g_pMtx;
    };
    std::future<void> fut = std::async(std::launch::async, delay_publish);

    zmqClient.get_next_message("publisher1", &msg);
    ulOuter.unlock();
}
