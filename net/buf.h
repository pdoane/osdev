// ------------------------------------------------------------------------------------------------
// net/buf.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/link.h"

// ------------------------------------------------------------------------------------------------
// Net Buffer

#define NET_BUF_SIZE        2048
#define NET_BUF_START       256     // Room for various protocol headers + header below

typedef struct Net_Buf
{
    Link            link;
    u8*             start;          // offset to data start
    u8*             end;            // offset to data end exclusive
    uint            ref_count;
    u32             seq;            // Data from TCP header used for out-of-order/retransmit
    u8              flags;          // Data from TCP header used for out-of-order/retransmit
} Net_Buf;

// ------------------------------------------------------------------------------------------------
// Globals

extern int net_buf_alloc_count;

// ------------------------------------------------------------------------------------------------
// Functions

Net_Buf* net_alloc_buf();
void net_release_buf(Net_Buf* buf);
