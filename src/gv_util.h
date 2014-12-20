#ifndef _GV_UTIL_H_
#define _GV_UTIL_H_

#include "gv_type.h"

#define GV_ERROR_BUF_SIZE (64)

char p_error_buffer[GV_ERROR_BUF_SIZE];

#ifdef GV_DEBUG
#define GV_DEBUG_PRINT(fmt, args...)                        \
    do                                                      \
    {                                                       \
        fprintf(stderr, "GV_DEBUG: %s:%d:%s(): " fmt "\n",  \
                __FILE__, __LINE__, __func__, ##args);      \
    } while (0)
#else
#define GV_DEBUG_PRINT(fmt, args...)
#endif

char *gv_error_string(GV_ERROR err);

#define BAIL_ON_GV_ERROR(err)                               \
    do                                                      \
    {                                                       \
        if (GV_ERROR_SUCCESS != (err))                      \
        {                                                   \
            GV_DEBUG_PRINT("%s", gv_error_string((err)));   \
            goto error;                                     \
        }                                                   \
    } while (0)

int gv_safe_free(void *mem);
char *gv_error_string(GV_ERROR err);

#endif

