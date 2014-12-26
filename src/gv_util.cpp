#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gv_type.hpp"
#include "gv_util.hpp"


int
gv_safe_free(void **mem)
{
    if (NULL == mem)
    {
        assert(false); // FIXME do I really want to exit like this?
    }
    if (NULL != *mem)
    {
        free(*mem);
        *mem = NULL;
        return 0;
    }
    return -1;
}

