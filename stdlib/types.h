// ------------------------------------------------------------------------------------------------
// stdlib/types.h
// ------------------------------------------------------------------------------------------------

#pragma once

typedef char                i8;
typedef unsigned char       u8;
typedef short               i16;
typedef unsigned short      u16;
typedef int                 i32;
typedef unsigned int        u32;
typedef long long           i64;
typedef unsigned long long  u64;

#ifdef CROSS
typedef u32                 size_t;
typedef u64                 uintptr_t;
#else
#include <stdlib.h>
#endif

typedef unsigned int        uint;

#define PACKED __attribute__((__packed__))
#define typeof __typeof__

#define bool _Bool
#define true 1
#define false 0
