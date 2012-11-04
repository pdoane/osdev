// ------------------------------------------------------------------------------------------------
// net/arp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/addr.h"
#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// ARP Header

typedef struct ArpHeader
{
    u16 htype;
    u16 ptype;
    u8 hlen;
    u8 plen;
    u16 op;
} PACKED ArpHeader;

// ------------------------------------------------------------------------------------------------
// Functions

void ArpInit();

const EthAddr *ArpLookupEthAddr(const Ipv4Addr *pa);
void ArpRequest(NetIntf *intf, const Ipv4Addr *tpa, u16 etherType, NetBuf *pkt);
void ArpReply(NetIntf *intf, const EthAddr *tha, const Ipv4Addr *tpa);

void ArpRecv(NetIntf *intf, NetBuf *pkt);
