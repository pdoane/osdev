// ------------------------------------------------------------------------------------------------
// net/buf.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/link.h"

// ------------------------------------------------------------------------------------------------
// Net Buffer

#define NET_BUF_SIZE        2048
#define NET_BUF_START       256     // Room for various protocol headers + header below

typedef struct NetBuf
{
    Link            link;
    u8             *start;          // offset to data start
    u8             *end;            // offset to data end exclusive
    uint            refCount;
    u32             seq;            // Data from TCP header used for out-of-order/retransmit
    u8              flags;          // Data from TCP header used for out-of-order/retransmit
} NetBuf;

// ------------------------------------------------------------------------------------------------
// Globals

extern int g_netBufAllocCount;

// ------------------------------------------------------------------------------------------------
// Functions

NetBuf *NetAllocBuf();
void NetReleaseBuf(NetBuf *buf);
