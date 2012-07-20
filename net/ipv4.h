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

typedef struct IPv4_Header
{
    u8 ver_ihl;
    u8 tos;
    u16 len;
    u16 id;
    u16 offset;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    IPv4_Addr src;
    IPv4_Addr dst;
} PACKED IPv4_Header;

// ------------------------------------------------------------------------------------------------
// Functions

void ipv4_rx(Net_Intf* intf, Net_Buf* pkt);
void ipv4_tx(const IPv4_Addr* dst_addr, u8 protocol, Net_Buf* pkt);
void ipv4_tx_intf(Net_Intf* intf, const IPv4_Addr* next_addr,
    const IPv4_Addr* dst_addr, u8 protocol, Net_Buf* pkt);

void ipv4_print(const Net_Buf* pkt);
