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
        assert(false); // FIXME do we really want to exit like this?
    }
    if (NULL != *mem)
    {
        free(*mem);
        *mem = NULL;
        return 0;
    }
    return -1;
}

int
gv_safe_delete(void **mem)
{
    if (NULL == mem)
    {
        assert(false); // FIXME do we really want to exit like this?
    }
    if (NULL != *mem)
    {
        delete *mem;
        *mem = NULL;
        return 0;
    }
    return -1;
}

