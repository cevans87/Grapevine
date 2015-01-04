#include <stdio.h>
#include <assert.h>
#include <dns_sd.h>

#include "gv_type.hpp"
#include "gv_util.hpp"
#include "grapevine.hpp"

//static int iTimeOut = 100000000;

//static void
//gv_handle_events(
//    IN DNSServiceRef serviceRef);

//static void
//gv_handle_events(
//    IN DNSServiceRef serviceRef)
//{
//    int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
//    int nfds = dns_sd_fd + 1;
//    fd_set readfds;
//    struct timeval tv;
//    int result;
//    bool bStopNow = false;
//
//    while(!bStopNow)
//    {
//        FD_ZERO(&readfds);
//        FD_SET(dns_sd_fd, &readfds);
//        tv.tv_sec = iTimeOut;
//        tv.tv_usec = 0;
//        result = select(nfds, &readfds, reinterpret_cast<fd_set *>(NULL),
//                reinterpret_cast<fd_set *>(NULL), &tv);
//        if (0 < result)
//        {
//            DNSServiceErrorType err = kDNSServiceErr_NoError;
//            if (FD_ISSET(dns_sd_fd, &readfds))
//            {
//                err = DNSServiceProcessResult(serviceRef);
//            }
//            if (err)
//            {
//                bStopNow = true;
//            }
//        }
//    }
//}

#ifdef GV_MAIN
int
main(int argc, char *argv[])
{
    char *p_error = NULL;

    GV_ERROR error = 2;
    assert(0 != argc);
    assert(NULL != argv);

    printf("%s\n", gv_error_strings[0]);
    printf("%i\n", GV_NUM_ERRORS);
    BAIL_ON_GV_ERROR(error);

    return 0;

out:
    return 0;

error:
    goto out;
}
#endif
