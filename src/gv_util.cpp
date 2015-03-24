#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gv_type.hpp"
#include "gv_util.hpp"


template <typename T>
int
gv_safe_free(T **mem)
{
    if (nullptr == mem)
    {
        assert(false); // FIXME do we really want to exit like this?
    }
    if (nullptr != *mem)
    {
        free(*mem);
        *mem = nullptr;
        return 0;
    }
    return -1;
}

template <typename T>
int
gv_safe_delete(T **mem)
{
    if (nullptr == mem)
    {
        assert(false); // FIXME do we really want to exit like this?
    }
    if (nullptr != *mem)
    {
        delete *mem;
        *mem = nullptr;
        return 0;
    }
    return -1;
}

