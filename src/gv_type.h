#ifndef GRAPEVINE_TEST_GV_TYPE_H_
#define GRAPEVINE_TEST_GV_TYPE_H_

#include <memory>

// Simple expansion for GV_ERROR enum definition. Redefine later with #
// expansion for gv_error_strings definition.
#define GV_ERROR_SYMBOL(symbol) symbol

// All error symbols defined here and only here.
#define GV_ERROR_SYMBOLS                                    \
    GV_ERROR_SYMBOL(GV_ERROR_SUCCESS),                      \
    GV_ERROR_SYMBOL(GV_ERROR_INVALID_ARG),                  \
    GV_ERROR_SYMBOL(GV_ERROR_NOOP),                         \
    GV_ERROR_SYMBOL(GV_ERROR_LOCK_UNAVAILABLE),             \
    GV_ERROR_SYMBOL(GV_ERROR_CHANNEL_FULL),                 \
    GV_ERROR_SYMBOL(GV_ERROR_CHANNEL_EMPTY),                \
    GV_ERROR_SYMBOL(GV_ERROR_CHANNEL_CLOSED),               \
    GV_ERROR_SYMBOL(GV_ERROR_ALREADY_ENABLED),              \
    GV_ERROR_SYMBOL(GV_ERROR_ALREADY_DISABLED),             \
    GV_ERROR_SYMBOL(GV_ERROR_EMFILE),                       \
    GV_ERROR_SYMBOL(GV_NUM_ERRORS)        // KEEP IN LAST PLACE!

enum GV_ERROR : int
{
    GV_ERROR_SYMBOLS
};

#ifndef GV_DEBUG
char const * const gv_error_strings[] = {NULL};
#else
// Redefine error symbol expansion for error code to string array definition.
#undef GV_ERROR_SYMBOL
#define GV_ERROR_SYMBOL(symbol) #symbol

char const * const gv_error_strings[] =
{
     GV_ERROR_SYMBOLS
};
#endif // GV_DEBUG

#endif // GRAPEVINE_TEST_GV_TYPE_H_

