// ------------------------------------------------------------------------------------------------
// eth.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "net_intf.h"

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
    const Eth_Header* hdr;
    u16 ether_type;
    const u8* data;
    uint data_len;
} Eth_Packet;

// ------------------------------------------------------------------------------------------------
// Functions

void eth_intf_init(Net_Intf* intf);

void eth_rx(Net_Intf* intf, u8* pkt, uint len);
void eth_tx(Net_Intf* intf, const Eth_Addr* dst_addr, u16 ether_type, u8* buf, uint len);
void eth_tx_ipv4(Net_Intf* intf, const IPv4_Addr* dst_addr, u8* buf, uint len);

void eth_print(const Eth_Packet* ep);
