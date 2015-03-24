#ifndef GRAPEVINE_SRC_GV_UTIL_HPP_
#define GRAPEVINE_SRC_GV_UTIL_HPP_
#include <stdio.h>

// Parameter input/output hints for function prototypes and definitions.
#define IN
#define OUT

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

template <typename T>
int gv_safe_free(T **mem);

template <typename T>
int gv_safe_delete(T **mem);

#endif // GRAPEVINE_SRC_GV_UTIL_HPP_

