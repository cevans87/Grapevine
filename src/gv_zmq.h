#ifndef GRAPEVINE_SRC_GV_ZMQ_H_
#define GRAPEVINE_SRC_GV_ZMQ_H_

#include <map>
#include <zmq.hpp>

#include "gv_util.h"

class ZMQClient
{
    public:
        ZMQClient();

    private:
        zmq::context_t _context;

};

#endif // GRAPEVINE_SRC_GV_ZMQ_H_

