// ------------------------------------------------------------------------------------------------
// eth.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"

// ------------------------------------------------------------------------------------------------
// Ethertypes

#define ET_IPV4                         0x0800
#define ET_ARP                          0x0806
#define ET_IPV6                         0x86DD

// ------------------------------------------------------------------------------------------------
// Ethernet Address

typedef struct Eth_Addr
{
    u8 n[6];
} PACKED Eth_Addr;

// ------------------------------------------------------------------------------------------------
// Ethernet Packet

typedef struct Eth_Packet
{
    const Eth_Addr* dst_addr;
    const Eth_Addr* src_addr;
    u16 ether_type;
    const u8* data;
    uint data_len;
} Eth_Packet;

// ------------------------------------------------------------------------------------------------
// Functions

bool eth_decode(Eth_Packet* ep, const u8* pkt, uint len);
u8* eth_encode_hdr(u8* pkt, const Eth_Addr* dst_mac, const Eth_Addr* src_mac, u16 ether_type);

void eth_addr_to_str(char* str, size_t size, const Eth_Addr* addr);
void eth_print(const Eth_Packet* ep);
