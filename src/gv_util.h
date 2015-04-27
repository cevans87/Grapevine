#ifndef GRAPEVINE_SRC_GV_UTIL_H_
#define GRAPEVINE_SRC_GV_UTIL_H_
#include <stdio.h>
#include <string.h>

#include "gv_cpp14_make_unique.h"

namespace grapevine {

#ifndef GV_DEBUG_LEVEL
#define GV_DEBUG_LEVEL (0)
#endif // GV_DEBUG_LEVEL

// Parameter input/output hints for function prototypes and definitions.
#define IN
#define OUT
#define INOUT

// Simple expansion for GV_DEBUG enum definition. Redefine later with #
// expansion for gv_error_strings definition.
#define GV_DEBUG_SYMBOL(symbol) symbol

// All debug symbols defined here and only here.
#define GV_DEBUG_SYMBOLS                                        \
    GV_DEBUG_SYMBOL(GV_DEBUG_OFF),                              \
    GV_DEBUG_SYMBOL(GV_DEBUG_EXPECTED),                         \
    GV_DEBUG_SYMBOL(GV_DEBUG_INFO),                             \
    GV_DEBUG_SYMBOL(GV_DEBUG_ENTRY),                            \
    GV_DEBUG_SYMBOL(GV_DEBUG_WARNING),                          \
    GV_DEBUG_SYMBOL(GV_DEBUG_DEBUG),                            \
    GV_DEBUG_SYMBOL(GV_DEBUG_ERROR),                            \
    GV_DEBUG_SYMBOL(GV_DEBUG_SEVERE),                           \
    GV_DEBUG_SYMBOL(GV_NUM_DEBUG)           // KEEP IN LAST PLACE!

enum GV_DEBUG : int
{
    GV_DEBUG_SYMBOLS
};

#if GV_DEBUG_LEVEL < 1
char const * const gv_debug_strings[] = {NULL};
#else
// Redefine error symbol expansion for error code to string array definition.
#undef GV_DEBUG_SYMBOL
#define GV_DEBUG_SYMBOL(symbol) (strchr(strchr(#symbol, '_') + 1, '_') + 1)

char const * const gv_debug_strings[] =
{
     GV_DEBUG_SYMBOLS
};

#endif // GV_DEBUG_LEVEL

// Simple expansion for GV_ERROR enum definition. Redefine later with #
// expansion for gv_error_strings definition.
#define GV_ERROR_SYMBOL(symbol) symbol

// All error symbols defined here and only here.
#define GV_ERROR_SYMBOLS                                                    \
    GV_ERROR_SYMBOL(GV_ERROR_SUCCESS),                                      \
    GV_ERROR_SYMBOL(GV_ERROR_NOOP),                                         \
    GV_ERROR_SYMBOL(GV_ERROR_LOCK_UNAVAILABLE),                             \
    GV_ERROR_SYMBOL(GV_ERROR_CHANNEL_FULL),                                 \
    GV_ERROR_SYMBOL(GV_ERROR_CHANNEL_EMPTY),                                \
    GV_ERROR_SYMBOL(GV_ERROR_CHANNEL_CLOSED),                               \
    GV_ERROR_SYMBOL(GV_ERROR_ALREADY_ENABLED),                              \
    GV_ERROR_SYMBOL(GV_ERROR_ALREADY_DISABLED),                             \
    GV_ERROR_SYMBOL(GV_ERROR_INVALID_ARG),                                  \
    GV_ERROR_SYMBOL(GV_ERROR_KEY_CONFLICT),                                 \
    GV_ERROR_SYMBOL(GV_ERROR_EMFILE),                                       \
    GV_ERROR_SYMBOL(GV_NUM_ERRORS)                      // KEEP IN LAST PLACE!

enum GV_ERROR : int
{
    GV_ERROR_SYMBOLS
};

#if GV_DEBUG_LEVEL < 1
char const * const gv_error_strings[] = {NULL};
#else
// Redefine error symbol expansion for error code to string array definition.
#undef GV_ERROR_SYMBOL
#define GV_ERROR_SYMBOL(symbol) (strchr(strchr(#symbol, '_') + 1, '_') + 1)
//#define GV_ERROR_SYMBOL(symbol) #symbol

char const * const gv_error_strings[] =
{
     GV_ERROR_SYMBOLS
};

#endif // GV_DEBUG_LEVEL

#if GV_DEBUG_LEVEL < 1
#define GV_DEBUG_PRINT(fmt, ...)
#define GV_DEBUG_PRINT_SEV(severity, fmt, ...)
#else
#define GV_FILE (strrchr(__FILE__, '/') ? \
        strrchr(__FILE__, '/') + 1 : __FILE__)

#define GV_DEBUG_PRINT_SEV(severity, fmt, ...)                              \
    do {                                                                    \
        if (GV_DEBUG_LEVEL <= (severity)) {                                 \
            fprintf(stderr, "%s:%d:%s(): %s: " fmt "\n",                    \
                    GV_FILE, __LINE__,                                      \
                    __func__, gv_debug_strings[(severity)], ##__VA_ARGS__); \
        }                                                                   \
    } while (0)

#define GV_DEBUG_PRINT(fmt, ...)                                            \
    GV_DEBUG_PRINT_SEV(GV_DEBUG_DEBUG, fmt, ##__VA_ARGS__)

#endif // GV_DEBUG_LEVEL

#define BAIL_ON_GV_ERROR_SEV(err, severity)                                 \
    do {                                                                    \
        if (GV_ERROR_SUCCESS != (err)) {                                    \
            if (GV_DEBUG_LEVEL >= severity) {                               \
                GV_DEBUG_PRINT_SEV(severity,                                \
                        "bail on error: %s",                                \
                        gv_error_strings[(err)]);                           \
            }                                                               \
            goto error;                                                     \
        }                                                                   \
    } while (0)

#define BAIL_ON_GV_ERROR_WARNING(err)                                       \
    BAIL_ON_GV_ERROR_SEV(err, GV_DEBUG_WARNING)

// TODO rebuild this macro to allow for a list of expected errors and just keep
// those from printing. Maybe call it BAIL_ON_GV_ERROR_HANDLED
#define BAIL_ON_GV_ERROR_EXPECTED(err)                                      \
    BAIL_ON_GV_ERROR_SEV(err, GV_DEBUG_EXPECTED)

#define BAIL_ON_GV_ERROR(err)                                               \
    BAIL_ON_GV_ERROR_SEV(err, GV_DEBUG_DEBUG)

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_UTIL_H_

