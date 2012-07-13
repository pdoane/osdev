// ------------------------------------------------------------------------------------------------
// net/buf.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/link.h"

// ------------------------------------------------------------------------------------------------
// Net Buffer

typedef struct NetBuf
{
    Link link;

    // Allow for up to 120 bytes to be used in the packet header for various protocols.  This allows
    // for IPv4 and TCP to both use all but two optional fields.
    u8 proto_headers[120];
} NetBuf;

#define MAX_PACKET_SIZE         1500

// ------------------------------------------------------------------------------------------------
// Functions

NetBuf* net_alloc_packet();
void net_free_packet(NetBuf* buf);
