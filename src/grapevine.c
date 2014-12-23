#include <stdio.h>
#include <assert.h>

#include "gv_type.h"
#include "gv_util.h"

#ifdef GV_MAIN
INT
main(INT argc, char *argv[])
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
