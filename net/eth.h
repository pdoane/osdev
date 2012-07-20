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

typedef struct Eth_Header
{
    Eth_Addr dst;
    Eth_Addr src;
    u16 ether_type;
} PACKED Eth_Header;

// ------------------------------------------------------------------------------------------------
// Ethernet Packet

typedef struct Eth_Packet
{
    Eth_Header* hdr;
    u16 ether_type;
    u16 hdr_len;
} Eth_Packet;

// ------------------------------------------------------------------------------------------------
// Functions

void eth_rx(Net_Intf* intf, Net_Buf* pkt);
void eth_tx_intf(Net_Intf* intf, const void* dst_addr, u16 ether_type, Net_Buf* pkt);

void eth_print(Net_Buf* pkt);
