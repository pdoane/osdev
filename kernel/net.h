// ------------------------------------------------------------------------------------------------
// net.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// Packet Header Management

// Allow for up to 128 bytes to be used in the packet header for various protocols.  This allows
// for IPv4 and TCP to both use all but one optional field.

#define MAX_PACKET_HEADER       128
#define MAX_PACKET_SIZE         1500

// ------------------------------------------------------------------------------------------------
// Byte Order translation

static inline u16 net_swap16(uint n)
{
    return ((n & 0x00ff) << 8) | ((n & 0xff00) >> 8);
}

static inline u32 net_swap32(uint n)
{
    return ((n & 0x000000ff) << 24) |
           ((n & 0x0000ff00) << 8) |
           ((n & 0x00ff0000) >> 8) |
           ((n & 0xff000000) >> 24);
}

// ------------------------------------------------------------------------------------------------
// Globals

extern u8 net_trace;

// ------------------------------------------------------------------------------------------------
// Functions

void net_init();
void net_poll();
