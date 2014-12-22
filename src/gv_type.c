#include "gv_type.h"

// Redefine error symbol expansion for error code to string array definition.
#undef GV_ERROR_SYMBOL
#define GV_ERROR_SYMBOL(symbol) #symbol

char const * const gv_error_strings[] =
{
     GV_ERROR_SYMBOLS
};

