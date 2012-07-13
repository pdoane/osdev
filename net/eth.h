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
    u8* data;
} Eth_Packet;

// ------------------------------------------------------------------------------------------------
// Functions

void eth_rx(Net_Intf* intf, u8* pkt, u8* end);
void eth_tx_intf(Net_Intf* intf, const void* dst_addr, u16 ether_type, u8* pkt, u8* end);

void eth_print(u8* pkt, u8* end);
