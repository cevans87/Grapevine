#include <stdio.h>
#include <assert.h>

#include "gv_type.h"
#include "gv_util.h"

INT
main(INT argc, char *argv[])
{
    char *p_error;
    GV_ERROR error = 1;
    assert(0 != argc);
    assert(NULL != argv);

    p_error = gv_error_string(GV_ERROR_SUCCESS);
    BAIL_ON_GV_ERROR(error);

    printf("%s\n", p_error);
    return 0;

out:
    return 0;

error:
    goto out;
}
