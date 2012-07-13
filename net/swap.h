// ------------------------------------------------------------------------------------------------
// net/swap.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Byte Order translation

static inline u16 net_swap16(u16 x)
{
    return (x>>8) | (x<<8);
}

static inline u32 net_swap32(u32 x)
{
    return __builtin_bswap32(x);
}

static inline u64 net_swap64(u64 x)
{
    return __builtin_bswap64(x);
}
