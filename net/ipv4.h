// ------------------------------------------------------------------------------------------------
// net/ipv4.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/intf.h"

// ------------------------------------------------------------------------------------------------
// IP Protocols

#define IP_PROTOCOL_ICMP                1
#define IP_PROTOCOL_TCP                 6
#define IP_PROTOCOL_UDP                 17

// ------------------------------------------------------------------------------------------------
// IPv4 Header

typedef struct Ipv4Header
{
    u8 verIhl;
    u8 tos;
    u16 len;
    u16 id;
    u16 offset;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    Ipv4Addr src;
    Ipv4Addr dst;
} PACKED Ipv4Header;

// ------------------------------------------------------------------------------------------------
// Functions

void Ipv4Recv(NetIntf *intf, NetBuf *pkt);
void Ipv4Send(const Ipv4Addr *dstAddr, u8 protocol, NetBuf *pkt);
void Ipv4SendIntf(NetIntf *intf, const Ipv4Addr *nextAddr,
    const Ipv4Addr *dstAddr, u8 protocol, NetBuf *pkt);

void Ipv4Print(const NetBuf *pkt);
