#ifndef GRAPEVINE_INCLUDE_GRAPEVINE_H_
#define GRAPEVINE_INCLUDE_GRAPEVINE_H_
#include <zmq.hpp>

#include "gv_browser.hpp"

namespace grapevine {

enum GV_FLAG : unsigned int
{
    GV_FLAG_BLOCK           = 1 << 0,
    GV_FLAG_NO_BLOCK        = 1 << 1
};

class GV_Publisher
{
    public:
        int publish_message(GV_FLAG flags, char const *msg);
        int register_service(char const *srv);
    private:
        zmq::context_t _zmq_context;
        GV_Browser _gv_browser;
};

class GV_Subscriber
{
    public:
        int subscribe(char const &srv);
        int get_last_message(GV_FLAG flags, char &msg);
        int get_next_message(GV_FLAG flags, char &msg);
        int get_message_at(int idx, char &msg);
    private:
        zmq::context_t _zmq_context;
        GV_Browser _gv_browser;
};

} // namespace grapevine

#endif // GRAPEVINE_INCLUDE_GRAPEVINE_H_

