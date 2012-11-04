// ------------------------------------------------------------------------------------------------
// net/eth.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// Ethertypes

#define ET_IPV4                         0x0800
#define ET_ARP                          0x0806
#define ET_IPV6                         0x86DD

// ------------------------------------------------------------------------------------------------
// Ethernet Header

typedef struct EthHeader
{
    EthAddr dst;
    EthAddr src;
    u16 etherType;
} PACKED EthHeader;

// ------------------------------------------------------------------------------------------------
// Ethernet Packet

typedef struct EthPacket
{
    EthHeader *hdr;
    u16 etherType;
    u16 hdrLen;
} EthPacket;

// ------------------------------------------------------------------------------------------------
// Functions

void EthRecv(NetIntf *intf, NetBuf *pkt);
void EthSendIntf(NetIntf *intf, const void *dstAddr, u16 etherType, NetBuf *pkt);

void EthPrint(NetBuf *pkt);
