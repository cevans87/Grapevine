#ifndef GRAPEVINE_SRC_GV_COMMUNICATOR_HPP_
#define GRAPEVINE_SRC_GV_COMMUNICATOR_HPP_

#include <dns_sd.h>

#include "gv_type.h"

// FIXME cevans87: What should the callback give?
typedef int (*gv_callback)(void);
using gv_browse_callback = DNSServiceBrowseReply;

using UP_GV_Services = std::unique_ptr<std::vector< char *>>;

class GV_Communicator
{
    public:
        GV_Communicator();
        int enable();
        int disable();
        GV_Error subscribe(
                IN char const &service);
        GV_Error publish(
                IN char const &service);
        GV_Error get_services(
                OUT UP_GV_Services *upServices);
        GV_Error get_last_message(
                IN char const &service,
                OUT char *message);
        GV_Error publish_message(
                IN char const &service,
                IN char const &message);
    private:
        void handleEvents(DNSServiceRef serviceRef);
        // FIXME cevans87: make this non-static.
        static void DNSServiceBrowseCallback(
                IN DNSServiceRef service,
                IN DNSServiceFlags flags,
                IN uint32_t interfaceIndex,
                IN DNSServiceErrorType errorCode,
                IN const char *name,
                IN const char *type,
                IN const char *domain,
                IN void *context
                );
        void DNSServiceBrowseCallback(
                IN DNSServiceRef service,
                IN DNSServiceFlags flags,
                IN uint32_t interfaceIndex,
                IN DNSServiceErrorType errorCode,
                IN const char *name,
                IN const char *type,
                IN const char *domain
                );
        DNSServiceErrorType browse();
        DNSServiceRef _serviceRef;
        gv_browse_callback _callback;
        GV_Context mContext;
};

using UP_GV_Communicator = std::unique_ptr<GV_Communicator>;

#endif // GRAPEVINE_SRC_GV_COMMUNICATOR_HPP_

