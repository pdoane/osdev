// ------------------------------------------------------------------------------------------------
// net/udp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net/ipv4.h"

// ------------------------------------------------------------------------------------------------
// UDP Header

typedef struct UDP_Header
{
    u16 src_port;
    u16 dst_port;
    u16 len;
    u16 checksum;
} PACKED UDP_Header;

// ------------------------------------------------------------------------------------------------
// Functions

void udp_rx(Net_Intf* intf, const IPv4_Header* ip_hdr, Net_Buf* pkt);
void udp_tx(const IPv4_Addr* dst_addr, uint dst_port, uint src_port, Net_Buf* pkt);
void udp_tx_intf(Net_Intf* intf, const IPv4_Addr* dst_addr,
    uint dst_port, uint src_port, Net_Buf* pkt);

void udp_print(const Net_Buf* pkt);
