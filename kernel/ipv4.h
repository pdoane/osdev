// ------------------------------------------------------------------------------------------------
// ipv4.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net_intf.h"

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

void ipv4_rx(Net_Intf* intf, const u8* pkt, uint len);
void ipv4_tx(Net_Intf* intf, const IPv4_Addr* dst_addr, u8 protocol, u8* buf, uint len);

u16 ipv4_checksum(const u8* data, uint len);
void ipv4_print(const u8* pkt, uint len);
