#ifndef _GV_UTIL_H_
#define _GV_UTIL_H_

#include "gv_type.h"

#define GV_ERROR_BUF_SIZE (64)
//#define GV_ERROR_STRING(err) (#err)

//char p_error_buffer[GV_ERROR_BUF_SIZE];

#ifndef GV_DEBUG
#define GV_DEBUG_PRINT(fmt, ...)
#else
#define GV_DEBUG_PRINT(fmt, ...)                                            \
    do                                                                      \
    {                                                                       \
        fprintf(stderr, "GV_DEBUG: %s:%d:%s(): " fmt "\n",                  \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__);               \
    } while (0)
#endif

char *gv_error_string(GV_ERROR err);

#define BAIL_ON_GV_ERROR(err)                                               \
    do                                                                      \
    {                                                                       \
        if (GV_ERROR_SUCCESS != (err))                                      \
        {                                                                   \
            GV_DEBUG_PRINT("bail on error: %s",                             \
                    gv_error_strings[(err)]);                               \
            goto error;                                                     \
        }                                                                   \
    } while (0)

int gv_safe_free(void *mem);
//char *gv_error_string(GV_ERROR err);

//extern char *gv_error_string_mapping[GV_NUM_ERRORS];

#endif

