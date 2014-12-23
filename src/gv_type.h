#ifndef GV_TYPE_H
#define GV_TYPE_H

#include <stdint.h>
#include <stdbool.h>

typedef     int             INT;
typedef     unsigned int    UINT;
typedef     int32_t         INT32;
typedef     uint32_t        UINT32;
typedef     int64_t         INT64;
typedef     uint64_t        UINT64;

// Simple expansion for GV_ERROR enum definition. Redefine later with #
// expansion for gv_error_strings definition.
#define GV_ERROR_SYMBOL(symbol) symbol

// All error symbols defined here and only here.
#define GV_ERROR_SYMBOLS                                    \
    GV_ERROR_SYMBOL(GV_ERROR_SUCCESS),                      \
    GV_ERROR_SYMBOL(GV_ERROR_TEST),                         \
    GV_ERROR_SYMBOL(GV_NUM_ERRORS)        // KEEP IN LAST PLACE!

typedef enum
{
    GV_ERROR_SYMBOLS
} GV_ERROR;

extern char const * const gv_error_strings[];

#endif

