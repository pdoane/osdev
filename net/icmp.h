// ------------------------------------------------------------------------------------------------
// net/icmp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/ipv4.h"

void IcmpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt);

void IcmpEchoRequest(const Ipv4Addr *dstAddr, u16 id, u16 sequence,
    const u8 *data, const u8 *end);
