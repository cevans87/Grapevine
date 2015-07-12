#ifndef GRAPEVINE_SRC_GV_UTIL_H_
#define GRAPEVINE_SRC_GV_UTIL_H_
#include <stdio.h>
#include <string.h>

#include "gv_cpp14_compatibility.h"

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
#define GV_DEBUG_SYMBOLS                                                            \
    GV_DEBUG_SYMBOL(OFF),                                                           \
    GV_DEBUG_SYMBOL(EXPECTED),                                                      \
    GV_DEBUG_SYMBOL(ENTRY),                                                         \
    GV_DEBUG_SYMBOL(EXIT),                                                          \
    GV_DEBUG_SYMBOL(SPAM),                                                          \
    GV_DEBUG_SYMBOL(INFO),                                                          \
    GV_DEBUG_SYMBOL(WARNING),                                                       \
    GV_DEBUG_SYMBOL(DEBUG),                                                         \
    GV_DEBUG_SYMBOL(ERROR),                                                         \
    GV_DEBUG_SYMBOL(SEVERE),                                                        \
    GV_DEBUG_SYMBOL(GV_NUM_DEBUG)                               // KEEP IN LAST PLACE!

enum class GV_DEBUG : int
{
    GV_DEBUG_SYMBOLS
};

#if GV_DEBUG_LEVEL < 1
char const * const gv_debug_strings[] = {NULL};
#else
// Redefine error symbol expansion for error code to string array definition.
#undef GV_DEBUG_SYMBOL
#define GV_DEBUG_SYMBOL(symbol) #symbol

char const * const gv_debug_strings[] =
{
     GV_DEBUG_SYMBOLS
};

#endif // GV_DEBUG_LEVEL

// Simple expansion for GV_ERROR enum definition. Redefine later with #
// expansion for gv_error_strings definition.
#define GV_ERROR_SYMBOL(symbol) symbol

// All error symbols defined here and only here.
#define GV_ERROR_SYMBOLS                                                            \
    GV_ERROR_SYMBOL(SUCCESS),                                                       \
    GV_ERROR_SYMBOL(NOOP),                                                          \
    GV_ERROR_SYMBOL(LOCK_UNAVAILABLE),                                              \
    GV_ERROR_SYMBOL(CHANNEL_FULL),                                                  \
    GV_ERROR_SYMBOL(CHANNEL_EMPTY),                                                 \
    GV_ERROR_SYMBOL(CHANNEL_CLOSED),                                                \
    GV_ERROR_SYMBOL(ALREADY_ENABLED),                                               \
    GV_ERROR_SYMBOL(ALREADY_DISABLED),                                              \
    GV_ERROR_SYMBOL(INVALID_ARG),                                                   \
    GV_ERROR_SYMBOL(KEY_MISSING),                                                   \
    GV_ERROR_SYMBOL(KEY_CONFLICT),                                                  \
    GV_ERROR_SYMBOL(NO_FD),                                                         \
    GV_ERROR_SYMBOL(OSERROR),                                                       \
    GV_ERROR_SYMBOL(NUM_ERRORS)                                 // KEEP IN LAST PLACE!

enum class GV_ERROR : int
{
    GV_ERROR_SYMBOLS
};

#if GV_DEBUG_LEVEL < 1
char const * const gv_error_strings[] = {NULL};
#else
// Redefine error symbol expansion for error code to string array definition.
#undef GV_ERROR_SYMBOL
#define GV_ERROR_SYMBOL(symbol) #symbol

char const * const gv_error_strings[] =
{
    GV_ERROR_SYMBOLS
};

#define GV_DEBUG_DEBUG_COLOR(sev)                                                   \
        (GV_DEBUG::DEBUG   == GV_DEBUG::sev) ? "\x1b[1;33m"    :                    \
        (GV_DEBUG::WARNING == GV_DEBUG::sev) ? "\x1b[1;33m"    :                    \
        (GV_DEBUG::SPAM    == GV_DEBUG::sev) ? "\x1b[1;36m"    :                    \
        (GV_DEBUG::INFO    == GV_DEBUG::sev) ? "\x1b[1;32m"    :                    \
        (GV_DEBUG::ERROR   == GV_DEBUG::sev) ? "\x1b[1;31m"    :                    \
        (GV_DEBUG::SEVERE  == GV_DEBUG::sev) ? "\x1b[1;41;32m" :                    \
        (GV_DEBUG::ENTRY   == GV_DEBUG::sev) ? "\x1b[1;34m"    :                    \
        (GV_DEBUG::EXIT    == GV_DEBUG::sev) ? "\x1b[1;35m"    : ""                 \

#define GV_DEBUG_COLOR_OFF "\x1b[0m"

#endif // GV_DEBUG_LEVEL

#if GV_DEBUG_LEVEL < 1
#define GV_PRINT(sev, fmt, ...)
#else

#define GV_FILE (strrchr(__FILE__, '/') ? \
        strrchr(__FILE__, '/') + 1 : __FILE__)

// FIXME avoid seg fault if sev isn't defined.
#define GV_PRINT(sev, fmt, ...)                                                     \
    do {                                                                            \
        if (GV_DEBUG_LEVEL <= static_cast<int>(GV_DEBUG::sev)) {                    \
            fprintf(stderr, "%s:%d:%s(): %s%s%s: " fmt "\n",                        \
                    GV_FILE, __LINE__, __func__,                                    \
                    GV_DEBUG_DEBUG_COLOR(sev),                                      \
                    gv_debug_strings[static_cast<int>(GV_DEBUG::sev)],              \
                    GV_DEBUG_COLOR_OFF,                                             \
                    ##__VA_ARGS__);                                                 \
        }                                                                           \
    } while (0)

#endif // GV_DEBUG_LEVEL

#define GV_BAIL(err, sev)                                                           \
    do {                                                                            \
        if (GV_ERROR::SUCCESS != (err)) {                                           \
            if (GV_ERROR::NUM_ERRORS <= (err)) {                                    \
                GV_PRINT(SEVERE, "Invalid error code");                             \
            } else if (GV_DEBUG_LEVEL <= static_cast<int>(GV_DEBUG::sev)) {         \
                GV_PRINT(sev, "bailed on: %s",                                      \
                        gv_error_strings[static_cast<int>(err)]);                   \
            }                                                                       \
            goto error;                                                             \
        }                                                                           \
    } while (0)

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_UTIL_H_

