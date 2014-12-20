#include <stdio.h>

#include "gv_type.h"
#include "gv_util.h"

char *
gv_error_string(GV_ERROR err)
{
    snprintf(p_error_buffer, GV_ERROR_BUF_SIZE, "haha error");
    return p_error_buffer;
}

