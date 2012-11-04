// ------------------------------------------------------------------------------------------------
// net/checksum.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/addr.h"

// ------------------------------------------------------------------------------------------------
// Checksum Header

typedef struct ChecksumHeader
{
    Ipv4Addr src;
    Ipv4Addr dst;
    u8 reserved;
    u8 protocol;
    u16 len;
} PACKED ChecksumHeader;

// ------------------------------------------------------------------------------------------------
// Functions

u16 NetChecksum(const u8 *data, const u8 *end);
uint NetChecksumAcc(const u8 *data, const u8 *end, uint sum);
u16 NetChecksumFinal(uint sum);
