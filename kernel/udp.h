// ------------------------------------------------------------------------------------------------
// udp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net_intf.h"

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

void udp_rx(Net_Intf* intf, const u8* pkt, uint len);
void udp_tx(const IPv4_Addr* dst_addr, uint dst_port, uint src_port, u8* pkt, uint len);

void udp_print(const u8* pkt, uint len);
