#ifndef _GV_TYPE_H_
#define _GV_TYPE_H_

#include <stdint.h>

typedef     int             INT;
typedef     unsigned int    UINT;
typedef     int32_t         INT32;
typedef     uint32_t        UINT32;
typedef     int64_t         INT64;
typedef     uint64_t        UINT64;

// Simple expansion allows for enum definition. Redefine with # expansion later
// for error code to string array definition.
#define GV_ERROR_SYMBOL(symbol) symbol

// All error symbols defined here and only here.
#define GV_ERROR_SYMBOLS                                    \
    GV_ERROR_SYMBOL(GV_ERROR_SUCCESS),                      \
    GV_ERROR_SYMBOL(GV_ERROR_TEST),                         \
    GV_ERROR_SYMBOL(GV_NUM_ERRORS)        // THIS MUST BE LAST!

typedef enum
{
    GV_ERROR_SYMBOLS
} GV_ERROR;

extern char const * const gv_error_strings[];

#endif

