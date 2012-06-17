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
// IPv4 Route

typedef struct IPv4_Route
{
    Link link;
    IPv4_Addr dst;
    IPv4_Addr mask;
    IPv4_Addr gateway;
    Net_Intf* intf;
} IPv4_Route;

// ------------------------------------------------------------------------------------------------
// Globals

extern Link g_ipv4_route_table;

// ------------------------------------------------------------------------------------------------
// Functions

void ipv4_rx(Net_Intf* intf, const u8* pkt, uint len);
void ipv4_tx(const IPv4_Addr* dst_addr, u8 protocol, u8* pkt, uint len);
void ipv4_tx_intf(Net_Intf* intf, const IPv4_Addr* next_addr,
    const IPv4_Addr* dst_addr, u8 protocol, u8* pkt, uint len);

const IPv4_Route* ipv4_find_route(const IPv4_Addr* dst);
void ipv4_add_route(const IPv4_Addr* dst, const IPv4_Addr* mask, const IPv4_Addr* gateway, Net_Intf* intf);
void ipv4_print_route_table();

u16 ipv4_checksum(const u8* data, uint len);
void ipv4_print(const u8* pkt, uint len);
