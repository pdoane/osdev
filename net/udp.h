// ------------------------------------------------------------------------------------------------
// net/udp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/ipv4.h"

// ------------------------------------------------------------------------------------------------
// UDP Header

typedef struct UdpHeader
{
    u16 srcPort;
    u16 dstPort;
    u16 len;
    u16 checksum;
} PACKED UdpHeader;

// ------------------------------------------------------------------------------------------------
// Functions

void UdpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt);
void UdpSend(const Ipv4Addr *dstAddr, uint dstPort, uint srcPort, NetBuf *pkt);
void UdpSendIntf(NetIntf *intf, const Ipv4Addr *dstAddr,
    uint dstPort, uint srcPort, NetBuf *pkt);

void UdpPrint(const NetBuf *pkt);
