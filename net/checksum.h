// ------------------------------------------------------------------------------------------------
// net/checksum.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/addr.h"

// ------------------------------------------------------------------------------------------------
// Checksum Header

typedef struct Checksum_Header
{
    IPv4_Addr src;
    IPv4_Addr dst;
    u8 reserved;
    u8 protocol;
    u16 len;
} PACKED Checksum_Header;

// ------------------------------------------------------------------------------------------------
// Functions

u16 net_checksum(const u8* data, const u8* end);
uint net_checksum_acc(const u8* data, const u8* end, uint sum);
u16 net_checksum_final(uint sum);
