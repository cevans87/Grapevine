#include <mutex>
#include <chrono>

#include "gv_util.h"
#include "gv_communicator.h"

#include "gtest/gtest.h"

using std::this_thread::sleep_for;
using std::chrono::seconds;
using std::mutex;
using std::unique_lock;

namespace gv = grapevine;

static mutex *g_pMtx = nullptr;

TEST(communicator, sends_and_receives) {
    gv::Communicator com;
    char pszMsg[] = "here's a message!";
    zmq::message_t msg;

    if (nullptr == g_pMtx) {
        g_pMtx = new mutex();
    }
    unique_lock<mutex> ulOuter(*g_pMtx);

    com.make_publisher("communicator_Test");
    com.make_subscriber("communicator_Test");

    std::function<void(void)> delay_publish = [&com, &pszMsg]() {
        unique_lock<mutex> ulInner(*g_pMtx, std::defer_lock);
        while (!ulInner.try_lock()) {
            sleep_for(seconds(2));
            com.publish_message("communicator_Test", pszMsg);
            printf("publishing\n");
        }
        delete g_pMtx;
    };
    std::future<void> fut = std::async(std::launch::async, delay_publish);

    com.get_next_message("communicator_Test", &msg);
    ulOuter.unlock();
}
