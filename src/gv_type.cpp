#include "gv_type.hpp"

// Redefine error symbol expansion for error code to string array definition.
#undef GV_ERROR_SYMBOL
#define GV_ERROR_SYMBOL(symbol) #symbol

#ifndef GV_DEBUG
char const * const gv_error_strings[] = NULL;
#else
char const * const gv_error_strings[] =
{
     GV_ERROR_SYMBOLS
};
#endif // GV_DEBUG
