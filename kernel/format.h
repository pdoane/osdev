// ------------------------------------------------------------------------------------------------
// format.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "vararg.h"
#include "types.h"

int vsnprintf(char* str, size_t size, const char* format, va_list args);
int snprintf(char* str, size_t size, const char* format, ...);
